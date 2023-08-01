#ifndef __PROGTEST__
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <climits>
#include <cfloat>
#include <cassert>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <numeric>
#include <string>
#include <vector>
#include <array>
#include <iterator>
#include <set>
#include <list>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <queue>
#include <stack>
#include <deque>
#include <memory>
#include <functional>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <stdexcept>
#include <condition_variable>
#include <pthread.h>
#include <semaphore.h>
#include "progtest_solver.h"
#include "sample_tester.h"
using namespace std;
#endif /* __PROGTEST__ */

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
struct PackFromThread
{
    PackFromThread ( ACompany _company, AProblemPack _problemPack, bool _completed ) :
            company     ( _company ),
            problemPack ( _problemPack ),
            completed   ( _completed ) {};

    ACompany        company;
    AProblemPack    problemPack;
    bool            completed;
};

struct StructSolver
{
    StructSolver ()
    {
        solver = createProgtestSolver();
    }
    StructSolver (const StructSolver & other)
    {
        solver = other.solver;
        packsInSolver = other.packsInSolver;
    }

    AProgtestSolver                                             solver;
    unordered_map<ACompany, queue<shared_ptr<PackFromThread>>>  packsInSolver;
};

class COptimizer
{
public:

    static bool                        usingProgtestSolver                     ( void )
    {
        return true;
    }
    static void                        checkAlgorithm                          ( AProblem                              problem )
    {
        // dummy implementation if usingProgtestSolver() returns true
    }
    void                               start                                   ( int                                   threadCount )
    {
        counterOfWorkingThreads = threadCount;
        // create working threads
        for ( int i = 0; i < threadCount; ++i )
        {
            workingThreads.emplace_back(&COptimizer::work, this);
        }

        // creating input and output threads
        for ( auto company : companies )
        {
            inputThreads . emplace_back ( thread(&COptimizer::receive, this, company) );
            outputThreads . emplace_back ( thread(&COptimizer::submit, this, company) );
        }
    }
    void                               stop                                    ( void )
    {

        for ( auto & thread : inputThreads )
            thread . join ();

        for ( auto & thread : workingThreads )
            thread . join ();

        for ( auto & thread : outputThreads )
            thread . join ();
    }
    void                               addCompany                              ( ACompany                              company )
    {
        companies               . emplace_back ( company );
        outputProblemPacks      . insert ( {company , queue<shared_ptr<PackFromThread>> () } );
        outputCondVars          . insert ( {company, make_shared<condition_variable_any>()} );
        companyMutexes          . insert ( { company, make_shared<std::mutex> () });

    }
private:

    vector<ACompany>    companies;

    // threads
    vector<thread>                                                  workingThreads;
    vector<thread>                                                  inputThreads;
    vector<thread>                                                  outputThreads;

    // queus
    queue<shared_ptr<PackFromThread>>                               inputProblemPacks;
    unordered_map<ACompany , queue<shared_ptr<PackFromThread>>>     outputProblemPacks;
    StructSolver                                                    mainSolver = StructSolver();

    // mutexes and condVars
    unordered_map<ACompany, shared_ptr<condition_variable_any>>     outputCondVars;
    unordered_map<ACompany, shared_ptr<std::mutex>>                 companyMutexes;

    condition_variable_any                                          condVarForWork;
    std::mutex                                                      mutexInput;
    std::mutex                                                      mutexSolve;
    std::mutex                                                      mutexCounterOfEndedInputThreads;
    std::mutex                                                      mutexCounterOfEndedWorkingThreads;

    size_t                                                          counterOfEndedInputThreads = 0;
    size_t                                                          counterOfEndedWorkingThreads = 0;
    size_t                                                          counterOfWorkingThreads;


    void receive (ACompany company )
    {
        AProblemPack newProblemPack;
        shared_ptr<PackFromThread> newStructPackProblem;
        while ( true )
        {
            newProblemPack = company -> waitForPack ();
            newStructPackProblem = make_shared<PackFromThread>(company, newProblemPack, false );

            companyMutexes [company] -> lock ();
            outputProblemPacks [company] . push ( newStructPackProblem );
            companyMutexes [company] -> unlock ();

            mutexInput . lock ();
            inputProblemPacks . push ( newStructPackProblem );
            condVarForWork . notify_one ();
            mutexInput . unlock ();

            if ( newProblemPack == nullptr )
            {
                condVarForWork . notify_all();
                break;
            }
        }
    }


    void work ()
    {
        while ( true )
        {
            mutexSolve.lock();
            condVarForWork . wait ( mutexSolve, [&] { return !inputProblemPacks . empty () || counterOfEndedInputThreads == companies . size (); });

            if ( counterOfEndedInputThreads == companies . size () && inputProblemPacks . empty () )
            {
                mutexCounterOfEndedWorkingThreads . lock ();
                ++counterOfEndedWorkingThreads;
                mutexCounterOfEndedWorkingThreads . unlock ();

                if ( counterOfEndedWorkingThreads == counterOfWorkingThreads )
                {
                    mainSolver . solver -> solve ();
                    anounceSolving ( mainSolver . packsInSolver );
                }
                mutexSolve . unlock ();
                break;
            }

            mutexInput . lock ();
            auto newStructPackProblem = inputProblemPacks . front ();
            inputProblemPacks . pop ();
            mutexInput . unlock ();

            if ( newStructPackProblem -> problemPack == nullptr )
            {
                mutexCounterOfEndedInputThreads . lock ();
                ++counterOfEndedInputThreads;
                mutexCounterOfEndedInputThreads . unlock ();

                newStructPackProblem -> completed = true;
                outputCondVars [newStructPackProblem -> company] -> notify_one();
                mutexSolve . unlock ();
            } else {
                auto countOfProblems = newStructPackProblem -> problemPack  -> m_Problems . size();
                for ( long unsigned int i = 0; i < countOfProblems; )
                {
                    if ( !mainSolver.solver -> hasFreeCapacity() )
                    {
                        auto fullSolver = mainSolver;
                        mainSolver = StructSolver();

                        mutexSolve . unlock ();

                        // working one thread
                        fullSolver. solver -> solve ();

                        mutexSolve . lock ();

                        anounceSolving ( fullSolver.packsInSolver );
                    } else {
                        mainSolver.solver -> addProblem ( newStructPackProblem -> problemPack -> m_Problems [i++] );
                    }
                }
                auto iter = mainSolver. packsInSolver . find ( newStructPackProblem -> company );
                if ( iter == mainSolver . packsInSolver . end() )
                    mainSolver . packsInSolver . insert ( {newStructPackProblem -> company, queue<shared_ptr<PackFromThread>> {} } );
                mainSolver . packsInSolver [ newStructPackProblem -> company ] . push ( newStructPackProblem );
                if ( !mainSolver.solver -> hasFreeCapacity() )
                {
                    auto fullSolver = mainSolver;
                    mainSolver = StructSolver();

                    mutexSolve . unlock ();

                    // working one thread
                    fullSolver. solver -> solve ();
                    anounceSolving ( fullSolver.packsInSolver );
                } else {
                mutexSolve . unlock ();
                }
            }
        }
    }

    void submit (ACompany company )
    {
        while ( true )
        {
            companyMutexes[company] -> lock ();
            outputCondVars [company] -> wait ( *companyMutexes[company], [&] { return !outputProblemPacks [company] . empty () && outputProblemPacks[company] . front() -> completed; });

            auto newStructPackProblem = outputProblemPacks [company] . front ();
            outputProblemPacks [company] . pop ();
            companyMutexes[company] -> unlock ();

            if ( newStructPackProblem -> problemPack == nullptr )
            {
                condVarForWork . notify_all();
                break;
            }

            company -> solvedPack(newStructPackProblem -> problemPack);
            condVarForWork . notify_one();
        }
    }


    void anounceSolving ( unordered_map<ACompany, queue<shared_ptr<PackFromThread>>> & marks )
    {
        for ( auto iter = marks . begin (); iter != marks . end (); ++iter )
        {
            while ( !iter -> second . empty () )
            {
                iter -> second . front () -> completed = true;
                iter -> second . pop ();
            }
            outputCondVars [iter -> first] -> notify_one();
        }
    }

};

// TODO: COptimizer implementation goes here
//-------------------------------------------------------------------------------------------------------------------------------------------------------------
#ifndef __PROGTEST__
int                                    main                                    ( void )
{
    COptimizer optimizer;
    ACompanyTest  company = std::make_shared<CCompanyTest> ();
    ACompanyTest  company2 = std::make_shared<CCompanyTest> ();

    optimizer . addCompany ( company );
    optimizer . addCompany ( company2 );
    optimizer . start ( 10 );
    optimizer . stop  ();
    if ( ! company -> allProcessed () )
        throw std::logic_error ( "(some) problems were not correctly processsed" );
    if ( ! company2 -> allProcessed () )
        throw std::logic_error ( "(some) problems were not correctly processsed" );
    return 0;
}
#endif /* __PROGTEST__ */

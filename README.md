# Multithreaded rent
## Task
The problem to be solved is represented by an instance of the class CProblem, which contains customer demands and information about the number of pieces of rented tools. These problems are grouped into packs, each represented by the class CProblemPack, where each pack contains instances of problems to be solved for different types of rented tools.

The rental companies continuously submit packs of problems to be solved, and the communication between the solver and the rental companies is facilitated by the class CCompany. Your implementation will create two helper threads for each CCompany instance - one thread will be responsible for loading new packs of problems for solving, and the other thread will return the solved packs of problems to the rental companies while ensuring the correct order of submission. The COptimizer class encapsulates the entire computation process and controls the execution and suspension of the computation as well as the behavior of computational threads used for solving problems from all managed rental companies.

### The COptimizer class will be used following this scenario:

Create an instance of COptimizer.

Register rental companies by calling the addCompany method.

Start the computation by calling the start method with the number of workThreads as a parameter. These threads will be created and wait for incoming requests. Additionally, two communication threads will be created for each registered rental company, one for receiving requests and the other for returning results. The start method will return to the caller after starting the threads.

The communication threads of rental companies will continuously take requests by calling the corresponding waitForPack method. The thread for receiving requests will terminate when it receives an empty request (smart pointer containing nullptr).

The computation threads will receive problems from the communication threads and solve them. After solving a problem, the computation threads will pass it to the corresponding rental company's delivery thread.

The delivery threads will wait for solved problems and return them in the correct order using the solvedPack method. Solved problems must be returned immediately as it is possible based on the order (solved problems cannot be stored and returned at the end of the computation). The delivery thread will terminate when no more problems are received from the respective rental company (waitForPack returned nullptr) and all problems from that rental company have been solved and delivered.

At some point, the test environment will call the stop method. This call will wait for the completion of serving all rental companies, terminate the work threads, and return to the caller.
The COptimizer instance will be released.

### Used classes and their interfaces:

CInterval: A class representing a customer's request interval. It encapsulates the rental interval and the offered price. The interface contains m_From for the start of the interval, m_To for the end of the interval, and m_Payment for the offered price.

CProblem: A class representing one problem to be solved. It is an abstract class, and the implementation (specifically the implementation of its subclass) is provided in the test environment. The interface contains m_Count for the number of rented tool pieces (1, 2, or 3), m_Intervals for the list of rental requests from customers, m_MaxProfit to store the calculated maximum profit, and a constructor and an add method for simplifying the work.

CProblemPack: A class representing a pack of problems to be solved. It is an abstract class, and the implementation (specifically the implementation of its subclass) is provided in the test environment. The interface contains m_Problems for the array of problem instances to be solved and an add method for simplifying the work.

CCompany: A class representing a rental company. The class defines an interface, and the actual implementation is provided in the test environment as a subclass of CCompany. The interface includes waitForPack to get the next pack of problems from the rental company and solvedPack to submit the solved pack of problems back to the rental company. These methods should be called by the same (identical) thread for each instance of the rental company.

COptimizer: A class encapsulating the entire computation process. You will create this class and must adhere to the following public interface:

A constructor without parameters to initialize a new instance of the class without creating any threads.

An addCompany(x) method to register a rental company x.

A start(workThr) method to create communication threads for all registered rental companies and start workThr working threads. This method will return to the caller after starting the threads.

A stop method that waits for the completion of serving rental companies and terminates the created threads. After this, the call to stop returns to the caller.

An usingProgtestSolver() method that returns true if you use the provided solver CProgtestSolver for solving problems or false if you implement the entire computation using your own code. If this method returns true, the test environment will not use the COptimizer::checkAlgorithm(problem) method below (you can leave it empty). If the method returns false, the test environment will modify the behavior of the provided CProgtestSolver solver to intentionally fill incorrect results.

A class method checkAlgorithm(problem). This method is used to test the correctness of your own computation algorithm. The input is an instance of CProblem, and the method calculates the required values and sets the m_MaxProfit property in the problem instance. In addition to checking the correctness of the implemented algorithms, this method is used to calibrate the speed of your solution. The speed is adjusted based on the size of the input problems to ensure reasonable testing time. Implement this method only if you do not use the provided CProgtestSolver solver (i.e., if your COptimizer::usingProgtestSolver() method returns false).

CProgtestSolver: A class providing an interface for sequential solving of input problems. The actual implementation is provided as a subclass. Instances of CProgtestSolver are created using the factory function createProgtestSolver(). The class also solves the problems in batches. Each instance of CProgtestSolver has a specified capacity, which determines how many problem instances can be placed in it for solving. Instances of CProgtestSolver are used only once. If more problems need to be solved, a new instance of CProgtestSolver must be created. The interface of the class includes hasFreeCapacity() to check if more problem instances can be added, addProblem(x) to add a problem instance to be solved, and solve() to start the computation and calculate the result for the inserted instances. The solve() method will be called only once for a specific instance of CProgtestSolver, and any additional attempts to call the method will result in an error.

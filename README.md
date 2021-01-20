# SAT Solving Problem 2

### GitHub:

> https://github.com/Finqen/SAT_Solving_Problem_2

## Remarks
All compulsory and bonus tasks have been implemented.
We added options to analyze the performance based on two criteria: "Execution time" (ET) & "Iteration steps" (IS).
ET refers to the wall clock time it took too solve the problem, while CS refers to the number of iteration until finished.

Remark: For smaller problems, heuristics had no significant benefit and introduced overhead which resulted in slower execution times.
On complex problems, heuristics, but specially preprocessing was essential!

Remark: We double check the solution found by our SAT solver by feeding in the solution afterwards and check if it does indeed solve the clause.

## Building & running the Project

For building the project just use the **CMakeLists.txt** in the top directory to create a Run configuration. <br> <br>
Per default, it runs all sat problems listed in the "/test" directory (both sat & unsat).
To change it so that it solves a specific problem, one can overwrite the list of paths e.g to:

```c
paths = {"../inputs/sat/uf50-08.cnf"};
```

To test e.g two or more specific problems, change to:

```c
paths = {"../inputs/sat/uf50-08.cnf","../inputs/sat/uf50-09.cnf"};
```

Executable can be found in the top directory of the project via <br>
> **/cmake-build-debug/SAT_Solver.exe** 

<br>

Solutions can be found in the top directory of the project via <br>
> **/cmake-build-debug/"filename".cnf** 

<br>

### Algortihm versions
We implemented several modifications to allow direct performance comparisions between them.
Per default, it runs our best found solution. To run different variants, the respective line can be changed to either:

```c
// Runs the default variant
for(const auto algorithm : Algorithm::Default) {...}
// Runs all variants (might be slow!)
for(const auto algorithm : Algorithm::All) {...}
// Runs variants that use pre-processing (is faster than ALL on complex problems)
for(const auto algorithm : Algorithm::WithPP) {...}
```
The following variants exist:
1. DEFAULT : "Uses a heuristic based on variable count."
2. NO_PREP : "As Default, but does NOT use pre-processing."
3. NOT_AUTARTIC : "As Default, but does NOT use autarc clause reduction."
4. NO_HEURISTIC : "As Default, but does NOT use any heuristic."
5. HEU_LIT : "Uses a heursitic based n literal occurences."

## Plotting the Cactus Plots

### Cactus Plot - Times
Generates a cactus plot based on the **execution time**.
```
 python plot-times.py
```

Output will be a **cactus-times.png** in the top directory.

<br>

### Cactus Plot - Steps
Generates a cactus plot based on the **iteration steps**.

```
 python plot-steps.py
```

Output will be a **cactus-steps.png** in the top directory.
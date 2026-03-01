# Basic Formulation of the Problem 

- A company executes a set of projects. Each project requires a certain human resources. For example, each project requires system architects, software architects, developers, testers. Each resource type has a certain cost per week. For example, system architect costs $1000 per week, software architect costs $800 per week, developer costs $500 per week, tester costs $300 per week. These projects are executed in a sequential manner, i.e., project 2 starts after project 1 is completed, project 3 starts after project 2 is completed and so on.

- For each project, a certain amount of work is planned for each resource type. For example, for project 1, 5 weeks of system architecture work is planned, 5 weeks of software architecture work is planned, 10 weeks of development work is planned, and 5 weeks of testing work is planned. The total duration of the project is the sum of the durations of the tasks for each resource type. For example, if project 1 requires 5 weeks of system architecture work, 5 weeks of software architecture work, 10 weeks of development work, and 5 weeks of testing work, then the total duration of project 1 is 5 + 5 + 10 + 5 = 25 weeks.

- We can reduce the duration of a project by increasing the amount of resources allocated to it. For example, if a project requires 10 weeks of development work, and we allocate 2 developers to it, then the duration of the development task for that project will be reduced to 5 weeks. This is because the work can be divided among the developers. However, there is a limit to how much we can reduce the duration of a project by allocating more resources to it. For example, if a project requires 10 weeks of development work, and we allocate 10 developers to it, then the duration of the development task for that project will be reduced to 1 week. However, it cannot be reduced further because the minimum hiring duration is 1 week.

- There is 1 resource of each resource type always available. There is a infinite pool of additional resources available but hiring each new resource increases the cost because the hiring is done on a short-term basis. For example, suppose there were 8 weeks allocated for a developer for a project (e.g., 8 * 500$). Now suppose, 1 more developer is added. The task can be completed within 4 weeks (i.e., 4 weeks saving). But each additional developer hired on short term basis costs double. So each additional developer costs additional 500$ for the duration of the work. So the first developer for 4 weeks cost 2000$ but the second developer costs 4000$. So there is a cost increase of 2000$ for a saving of 4 weeks. 

- The company has a certain additional budget that can be allocated to speed up the project execution. We want to find the optimal amount of additional resources to hire such that the total duration for the set of projects is minimized within the given additional budget.


## NOTE 

This optimization problem can be applied for different areas. For example, instead of projects we can think for hardware modules which are to implemented. Each module can be implemented using a set of hardware resources (e.g., adders, multiplers, logical operators etc.). Each resource has a certain cost (number of logic gates) and the execution time of the module can be reduced by using more resources. The goal is to minimize the total execution time within a given budget of additional gates.


# Formulation of the optimization Problem as ILP

- For a given task T, and for a given resource of type R, we create variables X_RT_1 to X_RT_N where N is maximum number of R resoures that can be allocated to T. X_RT_i is to be set 1 if i resources of type R are allocated to task T. Otherwise, it would be 0. 

- The time saving if i resources are allocated to task T is N - ceil(N/i). For example, if 10 weeks of system architecture work is needed, and 4 architects are assigned to this task then this task can be done in 3 weeks (ceil(10/4)). Hence, time saving is 10 - 3 = 7. Let us call this S_RT_i (S for saving).

- We want to maximize Sum(S_RT_i * X_RT_i) for all R in resources and all T in tasks. 

## Constraints: 

- At least one of X_RT_i must be one, i.e., Sum(X_RT_i) = 1 for a given resource R and a given task T 
- The total cost of additional resources allocated must be within the additional budget, i.e., Sum(C_RT_i * X_RT_i) ≤ C_additionalBudget where C_RT_i is the additional cost of allocating i resources of type R to task T. 


## Cost calculation 

- For X_RT_i for i is 0, because if there is no additional resource allocated, then there is no additional cost.

- For X_RT_i the total additional cost is (duration of task * cost per week for the additional resource). Duration of the task is given by ceil(N/i) where N is the original duration of the task. For X_RT_i the number of additional resources allocated is i - 1 because there is already 1 resource available. So the total cost is (ceil(N/i) * C_R * (i - 1)) where C_R is the cost per week for resource type R. 

- For example, if there are 10 weeks of development work and 4 developers are allocated, then the duration of the development task is ceil(10/4) = 3 weeks. The number of additional developers allocated is 4 - 1 = 3. If the cost per week for a developer is $500, then the total additional cost is (3 * 500 * 3) = $4500. 


# Formulation of a greedy heuristic 

- For all projects, compute the cost-benefit ratio for each X_RT_i. This value is given by additional costs per unit week saved (i.e., C_RT_i / S_RT_i).
- Select x_RT_i with the lowest cost-benefit ratio. If there are multiple such resources across all projects, then select the one with the lowest cost (and remove all other candidates for the same (R,T) pair). Then reduce C_additionalBudget by C_RT_i. Repeat this procedure till there is no extra budget available. 
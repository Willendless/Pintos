 Scheduling Lab

## 1 Scheduling Simulator Implementation

**(a) SRTF**

0: Arrival of Task 12 (ready queue length = 1)  
0: Run Task 12 for duration 2 (ready queue length = 0)  
1: Arrival of Task 13 (ready queue length = 1)  
2: Arrival of Task 14 (ready queue length = 2)
2: IO wait for Task 12 for duration 1
2: Run Task 14 for duration 1 (ready queue length = 1)
3: Arrival of Task 15 (ready queue length = 2)
3: Wakeup of Task 12 (ready queue length = 3)
3: IO wait for Task 14 for duration 2
3: Run Task 12 for duration 2 (ready queue length = 2)
5: Wakeup of Task 14 (ready queue length = 3)
5: Run Task 14 for duration 1 (ready queue length = 2)
6: Run Task 15 for duration 2 (ready queue length = 1)
8: Run Task 15 for duration 1 (ready queue length = 1)
9: Run Task 13 for duration 2 (ready queue length = 0)
11: Run Task 13 for duration 2 (ready queue length = 0)
13: Run Task 13 for duration 2 (ready queue length = 0)
15: Run Task 13 for duration 1 (ready queue length = 0)
16: Stop


**(b) MLFQ**

0: Arrival of Task 12 (ready queue length = 1)
0: Run Task 12 for duration 2 (ready queue length = 0)
1: Arrival of Task 13 (ready queue length = 1)
2: Arrival of Task 14 (ready queue length = 2)
2: IO wait for Task 12 for duration 1
2: Run Task 13 for duration 2 (ready queue length = 1)
3: Arrival of Task 15 (ready queue length = 2)
3: Wakeup of Task 12 (ready queue length = 3)
4: Run Task 14 for duration 1 (ready queue length = 3)
5: IO wait for Task 14 for duration 2
5: Run Task 15 for duration 2 (ready queue length = 2)
7: Wakeup of Task 14 (ready queue length = 3)
7: Run Task 12 for duration 2 (ready queue length = 3)
9: Run Task 14 for duration 1 (ready queue length = 2)
10: Run Task 13 for duration 4 (ready queue length = 1)
14: Run Task 15 for duration 1 (ready queue length = 1)
15: Run Task 13 for duration 1 (ready queue length = 0)
16: Stop

## 2 Approaching 100% Utilization

(a) This is an open system, since new tasks may arrive before old task completed

(b) We should choose $\frac{1}{M}$. Since the mean of exponential distribution is $\frac{1}{\lambda}$, then
$$\frac{1}{\lambda} = M$$,
so
$$\lambda = \frac{1}{M}$$

(c) We should choose $\frac{1}{2M}$. Same as (b), we have
$$\frac{1}{\lambda} = 2M$$,
so
$$\lambda = \frac{1}{2M}$$

(d) When $\lambda$ changes from small value to $\frac{1}{M}$, the CPU utilization increases linearly with steep slope and CPU utilization is about $50\%$ when arrival rate is $\frac{1}{2M}$, and $96\%$ when arrival rate is $\frac{1}{M}$, after $\lambda$ exceeds $\frac{1}{M}$ the CPU utilization increases more slowly and gradually approaches $100\%$
![404](1.png)

(e) 

1. For median response time, it stays 

![404](2.png)

(f)

(g)

## 3 Fairness for CPU Bursts

(a) Since there are two tasks, and 
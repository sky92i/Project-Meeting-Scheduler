# Project Meeting Scheduler (PMS)
Project of COMP2432

# Table of Contents
1. [Set up & Execution](#set-up-and-execution)
2. [Menu](#menu)
3. [Option 1 (Create Project Team)](#option1)
4. [Option 2 (Project Meeting booking)](#option2)
5. [Option 3 (Print Meeting Schedule)](#option3)
6. [Option 4 (Exit)](#option4)
7. [Some Input Examples for Testing](#input)

&nbsp;
## Set up and Execution
1. First compile the program using `gcc -o PMS G31_PMS.c`
2. Second execute the program by `./PMS`
3. A menu will be displayed for user input

&nbsp;
## Menu
The menu provides 4 options:
```
1 -- Create Project Teams
2 -- Project Meeting booking
3 -- Print Meeting Schedule
4 -- Exit
```
The user can use 0 to return to menu at anywhere.

&nbsp;
## Option 1 (Create Project Team) <a name="option1"></a>
Under option 1, the user can create project team

Input is according to the following format:
```
Team_Name Project_Name Project_Manager Project_Member1 Project_Member2(optional) Project_Member3(optional)
```
Example:
```
Team_A Project_A Alan Cathy Fanny Helen
```

However, staff in creation of teams violating the bellow will be invalid creation.
1. Each staff should only be 1 manager of 1 team.
2. Each staff should participate in at most 3 teams.

After a success creation of project team, the system will output:
```
>>>>>> Project Team_Name is created.
```

Else, the system will output:
```
>>>>>> Project %s is not created due to (manager/member) has exceed project limit
```

&nbsp;
## Option 2 (Project Meeting booking) <a name="option2"></a>
Under option 2, the user can create project meeting booking

The user can enter 2a / 2b to indicate their input as Single input / File input\
Input is according to the following format:
```
2a Team_Name Year-Month-Day Starting_Time Duration`
```
```
2b file_Name
```

Examples:
```
2a Team_A 2022-04-25 09:00 2
```
```
2b batch01_Requests.dat
```

All booking will be accepted by the program but may not be put into booking system if
1. Team does not exist
2. Invalid date
3. Booking out of working time
4. Inhumane duration

The system will output an acknowledgement after receiving a booking input:
```
>>>>>> Your (batch) request has been recorded.
```

&nbsp;
## Option 3 (Print Meeting Schedule) <a name="option3"></a>
Under option 3, the user can print meeting schedule

The user can enter 3a / 3b to indicate their meeting schedule by FCFS (first come first serve) / PRIO (priority scheduling)\
Input is according to the following format:
```
3a FCFS Start_Year-Start_Month-Start_Day End_Year-End_Month-End_Day
```
```
3b PRIO Start_Year-Start_Month-Start_Day End_Year-End_Month-End_Day
```

Examples:
```
3a FCFS 2022-04-25 2022-04-27
```
```
3b PRIO 2022-04-25 2022-04-30
```

The system will send an notice to indicate the output file name:
```
>>>>>> Printed. Export file name: Schedule_FCFS_01.txt.
```

&nbsp;
## Option 4 (Exit) <a name="option4"></a>
Under option 4, the user terminate the program and exit

The system will output an string to show that it has successfully terminated
```
Exited successfully
```

&nbsp;
## Some Input Examples for Testing <a name="input"></a>
```
Team_A Project_A Alan Cathy Fanny Helen
Team_B Project_B Cathy Helen Billy
Team_C Project_C Fanny Helen Eva
Team_D Project_D Eva Gary Billy
Team_E Project_E Gary Alan Billy

2a Team_A 2022-04-25 09:00 2
2a Team_A 2022-04-25 10:00 2
2a Team_A 2022-04-25 11:00 2
2a Team_B 2022-04-25 11:00 2
2a Team_C 2022-04-25 12:00 1
2a Team_D 2022-04-25 12:00 1
2a Team_E 2022-04-25 12:00 1
2a Team_A 2022-04-25 12:00 1


2a Team_A 2022-04-24 09:00 2
2a Team_A 2022-04-25 09:00 2
2a Team_A 2022-04-32 09:00 2

2a Team_A 2022-05-00 09:00 2
2a Team_A 2022-05-01 09:00 2
2a Team_A 2022-05-02 09:00 2
2a Team_A 2022-05-08 09:00 2
2a Team_A 2022-05-14 09:00 2
2a Team_A 2022-05-15 09:00 2

2a Team_A 2022-04-25 08:00 2
2a Team_A 2022-04-25 19:00 2
2a Team_A 2022-04-25 09:00 9
2a Team_A 2022-04-25 09:00 10
2a Team_A 2022-04-25 09:00 0
2a Team_A 2022-04-25 18:00 2
2a Team_A 2022-04-25 17:00 2

2b batch01_Requests.dat
2b batch02_Requests.dat
2b invalid_Requests.dat

3a FCFS 2022-04-25 2022-04-30
3a FCFS 2022-04-25 2022-05-14
3b PRIO 2022-04-25 2022-04-30
3b PRIO 2022-04-25 2022-05-14
```

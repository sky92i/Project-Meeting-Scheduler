Project Meeting Scheduler (PMS)

Contents of file
------------------------------------
* Member
* Set up & Execution
* Menu
* Option 1 (Create Project Team)
* Option 2 (Project Meeting booking)
* Option 3 (Print Meeting Schedule)
* Option 4 (Exit)


Member
-----------------------------------
Leung Chun Kiu Xavier 
LI, Yulin
NG, Ho Tin
PARK, Jungwon
YAM, Lok Yin


Set up & Execution
----------------------------------
1. First compile the program using "gcc -o PMS G31_PMS.c"
2. Second execute the program by "./PMS"
3. A menu will be displayed for user input


Menu
----------------------------------
The menu provides 4 options:
1 -- Create Project Teams
2 -- Project Meeting booking
3 -- Print Meeting Schedule
4 -- Exit

The user can use 0 to return to menu at anywhere


Option 1 (Create Project Team)
----------------------------------
Under option 1, the user can create project team

Input is according to the following format:
"Team_Name Project_Name Project_Manager Project_Member1 Project_Member2(optional) Project_Member3(optional)"
Examples are "Team_A Project_A Alan Cathy Fanny Helen"

However, staff in creation of teams violating the bellow will be invalid creation.
1. each staff should only be 1 manager of 1 team
2. Each staff should participate in at most 3 teams.

After a success creation of project team, the system will output:
">>>>>> Project Team_Name is created."

Else, the system will output:
">>>>>> Project %s is not created due to (manager/member) has exceed project limit"


Option 2 (Project Meeting booking)
----------------------------------
Under option 2, the user can create project meeting booking

The user can enter 2a / 2b to indicate their input as Single input / File input
Input is according to the following format:
"2a Team_Name Year-Month-Day Starting_Time Duration"
"2b file_Name"
Examples are "2a Team_A 2022-04-25 09:00 2"
& "2b batch01_Requests.dat"

All booking will be accepted by the program but may not be put into booking system if
1. Team does not exist
2. Invalid date
3. Booking out of working time
4. Inhumane duration

The system will output an acknowledgement after receiving a booking input:
">>>>>> Your (batch) request has been recorded."


Option 3 (Print Meeting Schedule)
----------------------------------
Under option 3, the user can print meeting schedule

The user can enter 3a / 3b to indicate their meeting schedule by FCFS (first come first serve) / PRIO (priority scheduling)
Input is according to the following format:
"3a FCFS Start_Year-Start_Month-Start_Day End_Year-End_Month-End_Day"
"3b PRIO Start_Year-Start_Month-Start_Day End_Year-End_Month-End_Day"
Examples are "3a FCFS 2022-04-25 2022-04-27"
& "3b PRIO 2022-04-25 2022-04-30"

The system will send an notice to indicate the output file name:
Examples are ">>>>>> Printed. Export file name: Schedule_FCFS_01.txt."


Option 4 (Exit)
----------------------------------
Under option 4, the user terminate the program and exit

The system will output an string to show that it has successfully terminated
"Exited successfully"

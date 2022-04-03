#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>

typedef struct
{
    char team[21];
    char project[21];
    int managerIndex; // initialized as boolean value (0/1) to check if isManager, can change to store name of project if needed
    int memberAIndex;
    int memberBIndex;
    int memberCIndex;
    //int availability[162] ;
} team;

typedef struct
{
    int teamsIndex;
    int date;
    int startTime;
    int duration;
} meeting;


meeting meetings[200] = {0};
team teams[20] = {0};
int fdp2c[4][2]; // parent -> child pipes
int fdc2p[4][2]; // child -> parent pipes
char *staff[] = {"Alan", "Billy", "Cathy", "David", "Eva", "Fanny", "Gary", "Helen"};
int managerCount[8] = {0};
int memberCount[8] = {0};
int staffCount = 8, slotsCount = 0, meetingsCount = 0, teamsCount = 0;
int rejectedMeetingIndex[200] = {-1}; // (FCFS)
int numberOfRejects = 0; // (FCFS)
int validDates[] = {25, 26, 27, 28, 29, 30, 2, 3, 4, 5, 6, 7, 9, 10, 11, 12, 13, 14};
meeting acceptedMeetingsApr[8][100] = {0}; // "8" stands for the number of staff members (FCFS)
meeting acceptedMeetingsMay[8][100] = {0}; // same as the above, not yet sorted by date and start time at the very beginning (FCFS)
meeting acceptedMeetingsInorder[8][200] = {0}; // combined of the meetings in both months, sorted by date and start time (FCFS)
int numAcceptedMeetingsApr[8] = {0}; // number of meetings accepted for each member (FCFS)
int numAcceptedMeetingsMay[8] = {0}; // (FCFS)

void clearCmmdBuf(char cmmd[30][30]) // updated
{
    int i;
    for (i = 0; i < 30; i++)
    {
        memset(cmmd[i], 0, 30);
    }
}

void sortMeetings(meeting unsortedMeetings[], int numberOfMeetings) // updated
{
    int minInex, i, j;
    for (i = 0; i < numberOfMeetings - 1; i++)
    {
        minInex = i;
        for (j = i + 1; j < numberOfMeetings; j++)
        {
            if (    (unsortedMeetings[j].date < unsortedMeetings[minInex].date) || // if the date of the first meeting is less than the second one's
                    (unsortedMeetings[j].date == unsortedMeetings[minInex].date && unsortedMeetings[j].startTime < unsortedMeetings[minInex].startTime)) // if they are on the same day, compare their start time
            {
                minInex = j;
            }
            // swapping of two meetings
            meeting dummy;
            memcpy(&dummy, &unsortedMeetings[minInex], sizeof(meeting));
            memcpy(&unsortedMeetings[minInex], &unsortedMeetings[i], sizeof(meeting));
            memcpy(&unsortedMeetings[i], &dummy, sizeof(meeting));
        }
    }
}

void mergeAprMay(meeting mergedMeetings[], meeting sortedMeetingsInApr[], meeting sortedMeetingsInMay[], int numberOfMeetingsInApr, int numberOfMeetingsInMay) // updated
{
    int i;
    // meetings in April are put first
    for (i = 0; i < numberOfMeetingsInApr; i++)
    {
        memcpy(&mergedMeetings[i], &sortedMeetingsInApr[i], sizeof(meeting));
    }
    for (i = 0; i < numberOfMeetingsInMay; i++)
    {
        memcpy(&mergedMeetings[i+numberOfMeetingsInApr], &sortedMeetingsInMay[i], sizeof(meeting));
    }
}

int searchStaffIndex(char *input) // updated
{
    int i;
    for (i = 0; i < staffCount; i++)
    {
        if (strcmp(staff[i], input) == 0) return i;
    }
    return -1;
}

int createMeeting(meeting meetings[], int meetingsCount, team teams[], int teamsCount, char *teamName, char *date,
                  char *startTime, char *duration)
{
    int i;
    char tmp[3] = {0};
    for (i = 0; i < teamsCount; i++)
    {
        if (strcmp(teams[i].team, teamName) == 0)
        {
            meetings[meetingsCount].teamsIndex = i;
        }
    }
    tmp[0] = date[8];
    tmp[1] = date[9];
    tmp[2] = 0; // find days in date e.g. 25 in 2022-04-25
    meetings[meetingsCount].date = atoi(tmp);
    tmp[0] = startTime[0];
    tmp[1] = startTime[1]; // find hours in startTime e.g. 09 in 09:00
    meetings[meetingsCount].startTime = atoi(startTime);
    meetings[meetingsCount].duration = atoi(duration);
    return 1;

}

int
creatTeam(team teams[], int index, int managerCount[], int memberCount[], char *team, char *project, int managerIndex,
          int memberAIndex, int memberBIndex, int memberCIndex)
{
    if (managerCount[managerIndex] == 1) return -1; // manager is already another project's manager
    if (memberAIndex != -1)
    { if (memberCount[memberAIndex] >= 3) return -2; }
    if (memberBIndex != -1)
    { if (memberCount[memberBIndex] >= 3) return -2; }
    if (memberCIndex != -1)
    { if (memberCount[memberCIndex] >= 3) return -2; }
    strcpy(teams[index].team, team);
    strcpy(teams[index].project, project);
    teams[index].managerIndex = managerIndex;
    managerCount[managerIndex]++;
    memberCount[managerIndex]++;
    teams[index].memberAIndex = memberAIndex;
    memberCount[memberAIndex]++;
    teams[index].memberBIndex = memberBIndex;
    memberCount[memberBIndex]++;
    teams[index].memberCIndex = memberCIndex;
    memberCount[memberCIndex]++;
    return 1;
}

void FCFS() // only to be called by FCFS child
{
    char recvCmmd[30][30]; // received command
    // only the meetings between 25/4 to 14/5 will be considered valid
    int apr[6][8][9] = {0}; // totally 6 working days in April, 8 members, 9 working hours per day
    int may[12][8][9] = {0}; // totally 12 working days in May
    // the first element of the staff on the day stands for the time 0900-1000, etc. the last element ([X][X][9]) stands for the time 1700-1800
    // considered boolean type, 0 stands for available, 1 stands for n/a
    while (1) // until a quit message is received from the parent
    {
        read(fdp2c[0][0], recvCmmd, 900 * sizeof(char));
        printf("Received from parent (FCFS): %s, %s, %s, %s, %s, %s, %s\n", recvCmmd[0], recvCmmd[1], recvCmmd[2],
               recvCmmd[3], recvCmmd[4], recvCmmd[5], recvCmmd[6]); // for debugging, showing what have been received
        if (strcmp(recvCmmd[0], "1") == 0)
        {
            int staffAindex = 0, staffBindex = 0, staffCindex = 0, staffDindex = 0;
            staffAindex = searchStaffIndex(recvCmmd[3]);
            staffBindex = searchStaffIndex(recvCmmd[4]);
            staffCindex = searchStaffIndex(recvCmmd[5]);
            staffDindex = searchStaffIndex(recvCmmd[6]);
            creatTeam(teams, teamsCount++, managerCount, memberCount, recvCmmd[1], recvCmmd[2], staffAindex,
                      staffBindex, staffCindex, staffDindex);
            // must be created successfully, validated by parent
//            printf("Team name: %s, Project: %s, Manger: %d, Number of teams: %d\n", teams[0].team, teams[0].project, teams[0].managerIndex, teamsCount);
        }
        else if (strcmp(recvCmmd[0], "2a") == 0) // create meetings
            // recvCmmd[0] = "2a"
            // recvCmmd[1] = "Team_A" (Team name)
            // recvCmmd[2] = "2022-04-25" (date)
            // recvCmmd[3] = "09:00" (time)
        {
            createMeeting(meetings, meetingsCount, teams, teamsCount, recvCmmd[1], recvCmmd[2], recvCmmd[3],
                          recvCmmd[4]);
            meetingsCount++;
        }
        else if (strcmp(recvCmmd[0], "3") == 0) // print meeting schedule
            // recvCmmd[0] = "3"
            // recvCmmd[1] = "FCFS"
            // recvCmmd[2] = "start date"
            // recvCmmd[3] = "end date"
        {
            int i;
            char currentTeam[21];
            int date;
            int startTime;
            int duration;
            int memberA, memberB, memberC; // indexes
            int startDate = atoi(recvCmmd[2]); // start date of the period
            int startDateIndex; // referring to the array validDates[]
            int endDate = atoi(recvCmmd[3]);
            int endDateIndex;
            // date adjustment
            if (startDate < 25 && startDate > 14)
            {
                startDate = 25;
                // the date will be considered 04-25
            }
            else if (startDate == 1 || startDate == 8)
            {
                startDate++; // the date will be considered 05-02 or 05-08
            }
            if (endDate < 25 && endDate > 14)
            {
                endDate = 14; // the date will be considered 05-14
            }
            else if (endDate == 1)
            {
                endDate = 30; // the date will be considered 04-30
            }
            else if (endDate == 8)
            {
                endDate = 7; // the date will be considered 05-07
            }
            for (i = 0; i < 18; i++) // 18 means there are 18 working days in total
            {
                if (startDate == validDates[i])
                {
                    startDateIndex = i;
                }
                if (endDate == validDates[i])
                {
                    endDateIndex = i;
                }
            }
            if (startDate > endDate)
            {
                printf("Error: the start date is greater than the end date\n");
                // should not be run
            }
            for (i = 0; i < meetingsCount; i++)
            {
                int j;
                int accepted = 1;
                strcpy(currentTeam,
                       teams[meetings[i].teamsIndex].team); // the team name of the team in the i-th booking
                date = meetings[i].date;
                startTime = meetings[i].startTime;
                duration = meetings[i].duration;
                memberA = teams[meetings[i].teamsIndex].memberAIndex;
                memberB = teams[meetings[i].teamsIndex].memberBIndex;
                memberC = teams[meetings[i].teamsIndex].memberCIndex;

                if (date >= 25 && date <= 30) // in April
                {
                    int dateIndex = date - 25; // offset: 25 --> 0, 26 --> 1, ... , 30 --> 5
                    int timeIndex = startTime -
                                    9; // offset: 0900 --> 0, 1000 --> 1, ... , 1700 --> 8. Remarks: its not allowed that the start time be 1800
                    for (j = 0; j < duration; j++)
                    {
                        if (apr[dateIndex][teams[meetings[i].teamsIndex].managerIndex][timeIndex + j] ==
                            1) // that timeslot of the manager is occupied
                        {
                            accepted = 0; // the booking is rejected
                            break; // break the for loop
                        }
                        else if (memberA != -1 && // memberA exists
                                 apr[dateIndex][memberA][timeIndex + j] == 1) // that timeslot of memberA is occupied
                        {
                            accepted = 0;
                            break;
                        }
                        else if (memberB != -1 &&
                                 apr[dateIndex][memberB][timeIndex + j] == 1)
                        {
                            accepted = 0;
                            break;
                        }
                        else if (memberC != -1 &&
                                 apr[dateIndex][memberC][timeIndex + j] == 1)
                        {
                            accepted = 0;
                            break;
                        }
                    }
                    if (accepted) // updating the meetings record and the time slots
                    {
                        memcpy(&acceptedMeetingsApr[teams[meetings[i].teamsIndex].managerIndex][numAcceptedMeetingsApr[teams[meetings[i].teamsIndex].managerIndex]++], &meetings[i],
                               sizeof(meeting)); // a meeting is accepted, update the related record for the manager
                        if (memberA != -1) // record-updating
                        {
                            memcpy(&acceptedMeetingsApr[memberA][numAcceptedMeetingsApr[memberA]++], &meetings[i],
                                   sizeof(meeting));
                        }
                        if (memberB != -1) // record-updating
                        {
                            memcpy(&acceptedMeetingsApr[memberB][numAcceptedMeetingsApr[memberB]++], &meetings[i],
                                   sizeof(meeting));
                        }
                        if (memberC != -1) // record-updating
                        {
                            memcpy(&acceptedMeetingsApr[memberC][numAcceptedMeetingsApr[memberC]++], &meetings[i],
                                   sizeof(meeting));
                        }
                        for (j = 0; j < duration; j++) // updating the time slots for each member
                        {
                            apr[dateIndex][teams[meetings[i].teamsIndex].managerIndex][timeIndex +
                                                                                       j] = 1; // timeslot occupied, manager
                            if (memberA != -1)
                            {
                                apr[dateIndex][memberA][timeIndex + j] = 1;
                            }
                            if (memberB != -1)
                            {
                                apr[dateIndex][memberB][timeIndex + j] = 1;
                            }
                            if (memberC != -1)
                            {
                                apr[dateIndex][memberC][timeIndex + j] = 1;
                            }
                        }
                    }
                    else
                    {
                        rejectedMeetingIndex[numberOfRejects++] = i;
                    }
                }
                else if (date >= 2 && date <= 14) // in May
                    // 8th is Sunday in May, neglected
                {
                    int dateIndex;
                    if (date <= 7)
                    {
                        dateIndex = date - 2; // offset: 2 --> 0, 3 --> 1, ... , 7 --> 5
                    }
                    else if (date >= 9)
                    {
                        dateIndex = date - 3; // offset: 9 --> 6, 10 --> 7, ... , 14 --> 11
                    }
                    int timeIndex = startTime - 9;
                    for (j = 0; j < duration; j++)
                    {
                        if (may[dateIndex][teams[meetings[i].teamsIndex].managerIndex][timeIndex + j] ==
                            1) // that timeslot of the manager is occupied
                        {
                            accepted = 0; // the booking is rejected
                            break; // break the for loop
                        }
                        else if (memberA != -1 && // memberA exists
                                 may[dateIndex][memberA][timeIndex + j] == 1) // that timeslot of memberA is occupied
                        {
                            accepted = 0;
                            break;
                        }
                        else if (memberB != -1 &&
                                 may[dateIndex][memberB][timeIndex + j] == 1)
                        {
                            accepted = 0;
                            break;
                        }
                        else if (memberC != -1 &&
                                 may[dateIndex][memberC][timeIndex + j] == 1)
                        {
                            accepted = 0;
                            break;
                        }
                    }
                    if (accepted) // update the meetings record and the timeslots
                    {
                        memcpy(&acceptedMeetingsMay[teams[meetings[i].teamsIndex].managerIndex][numAcceptedMeetingsMay[teams[meetings[i].teamsIndex].managerIndex]++], &meetings[i],
                               sizeof(meeting)); // a meeting is accepted, update the related record for the manager
                        if (memberA != -1) // record-updating
                        {
                            memcpy(&acceptedMeetingsMay[memberA][numAcceptedMeetingsMay[memberA]++], &meetings[i],
                                   sizeof(meeting));
                        }
                        if (memberB != -1) // record-updating
                        {
                            memcpy(&acceptedMeetingsMay[memberB][numAcceptedMeetingsMay[memberB]++], &meetings[i],
                                   sizeof(meeting));
                        }
                        if (memberC != -1) // record-updating
                        {
                            memcpy(&acceptedMeetingsMay[memberC][numAcceptedMeetingsMay[memberC]++], &meetings[i],
                                   sizeof(meeting));
                        }
                        for (j = 0; j < duration; j++)
                        {
                            may[dateIndex][teams[meetings[i].teamsIndex].managerIndex][timeIndex +
                                                                                       j] = 1; // timeslot occupied, manager
                            if (memberA != -1)
                            {
                                may[dateIndex][memberA][timeIndex + j] = 1;
                            }
                            if (memberB != -1)
                            {
                                may[dateIndex][memberB][timeIndex + j] = 1;
                            }
                            if (memberC != -1)
                            {
                                may[dateIndex][memberC][timeIndex + j] = 1;
                            }
                        }
                    }
                    else
                    {
                        rejectedMeetingIndex[numberOfRejects++] = i;
                    }
                }
                else // invalid date
                {
                    printf("Error\n");
                    // this statement is not supposed to be run under any circumstance
                    // as the date should be data-validated in the parent process
                }
            }
            // start to sort meetings, based on date and start time, merge all sorted meetings into one array finally
            for (i = 0; i < 8; i++) // 8 staff members in total
            {
                sortMeetings(acceptedMeetingsApr[i],numAcceptedMeetingsApr[i]); // meetings in April
                sortMeetings(acceptedMeetingsMay[i],numAcceptedMeetingsMay[i]); // meetings in May
                mergeAprMay(acceptedMeetingsInorder[i], acceptedMeetingsApr[i], acceptedMeetingsMay[i], numAcceptedMeetingsApr[i], numAcceptedMeetingsMay[i]);
            }
            // TODO: print the schedule to a txt file
            // printing on terminal, for testing, temporary codes
//            for (i = 0; i < numberOfRejects; i++)
//            {
//                int rejectedTeamIndex = meetings[rejectedMeetingIndex[i]].teamsIndex;
//                int rejectedDate = meetings[rejectedMeetingIndex[i]].date;
//                int rejectedStartTime = meetings[rejectedMeetingIndex[i]].startTime;
//                int rejectedDuration = meetings[rejectedMeetingIndex[i]].duration;
//                printf("Rejected: Team: %s, Date: %d, Start time: %d, Duration: %d\n", teams[rejectedTeamIndex].team,
//                       rejectedDate, rejectedStartTime, rejectedDuration);
//            }
//            for (i = 0; i < 8; i++)
//            {
//                int k, startHour = 9;
//                printf("==================================================================\n");
//                printf("Staff: %s\n", staff[i]);
//                for (k = 0; k < 9; k++) // 9 working hours
//                {
//                    printf("%d:00 - %d:00 : %d\n",startHour+k, startHour+k+1, apr[0][i][k]);
//                }
//            }
//            printf("Number of accepted meetings (Alan): %d\n",numAcceptedMeetingsApr[0]);
//            printf("Number of accepted meetings (Cathy): %d\n",numAcceptedMeetingsApr[2]);
//            printf("Start time: %d\n",acceptedMeetingsApr[0][0].startTime);
            printf("*** Project Meeting ***\n\n");
            printf("Algorithm used: FCFS\n");
            int startMonth;
            int endMonth;
            char stringStartDate[4];
            char stringEndDate[4];
            if (startDate >= 25 && startDate <= 30)
            {
                startMonth = 4;
                sprintf(stringStartDate, "%d", startDate);
            }
            else
            {
                startMonth = 5;
                strcpy(stringStartDate, "0");
                char dummyStr[3];
                sprintf(dummyStr, "%d", startDate);
                strcat(stringStartDate,dummyStr);
            }
            if (endDate >= 25 && endDate <= 30)
            {
                endMonth = 4;
                sprintf(stringEndDate, "%d", endDate);
            }
            else
            {
                endMonth = 5;
                strcpy(stringEndDate, "0");
                char dummStr[3];
                sprintf(dummStr, "%d", endDate);
                strcat(stringEndDate, dummStr);
            }
            printf("Period: 2022-0%d-%s to 2022-0%d-%s\n", startMonth, stringStartDate, endMonth, stringEndDate);
            for (i = 0; i < 8; i++) // 8 staff members
            {
                int j;
                printf("Date\t\t\tStart\t\t\tEnd\t\t\tTeam\t\t\tProject\n");
                printf("=========================================================================\n");
                for (j = startDateIndex; j < endDateIndex; j++)
                {
                    int k;
                    for (k = 0; k < numAcceptedMeetingsApr[i]+numAcceptedMeetingsMay[i]; k++)
                    {
                        int currentDate = acceptedMeetingsInorder[i][k].date;
                        int currentStart = acceptedMeetingsInorder[i][k].startTime;
                        int currentEnd = currentStart + acceptedMeetingsInorder[i][k].duration;
                        int currentTeamIndex = acceptedMeetingsInorder[i][k].teamsIndex;
                        if (currentDate != validDates[j])
                        {
                            continue;
                        }
                        char currentTeamName[21], currentProject[21]; // to be printed
                        char stringDate[21] = "2022-"; // to be printed
                        char dummyDate[10], dummyDate2[4];
                        char stringStartTime[10] = {0}, stringEndTime[10] = {0};
                        char dummyTime[6];
                        strcpy(currentTeamName, teams[currentTeamIndex].team);
                        strcpy(currentProject, teams[currentTeamIndex].project);
                        if (currentDate >= 25 && currentDate <= 30) // april
                        {
                            strcpy(dummyDate, "04-");
                        }
                        else // may
                        {
                            strcpy(dummyDate, "05-");
                        }
                        sprintf(dummyDate2, "%d", currentDate);
                        strcat(dummyDate, dummyDate2);
                        strcat(stringDate, dummyDate); // string date ok, to be printed
                        if (startTime == 9)
                        {
                            strcpy(stringStartTime, "0");
                        }
                        sprintf(dummyTime, "%d", currentStart);
                        strcat(stringStartTime, dummyTime);
                        strcat(stringStartTime, ":00"); // string start time ok, to be printed
                        sprintf(dummyTime, "%d", currentEnd);
                        strcat(stringEndTime, dummyTime);
                        strcat(stringEndTime, ":00"); // string end time ok, to be printed
                        printf("%s\t\t%s\t\t\t%s\t\t%s\t\t\t%s\n", stringDate, stringStartTime, stringEndTime, currentTeamName, currentProject);
                    }
                }
                printf("=========================================================================\n");
                printf("Staff: %s\n\n",staff[i]);
            }
        }
        else if (strcmp(recvCmmd[0], "end") == 0)
        {
            return;
        }
    }
}

int main()
{
    int i;
    char cmmd[30][30] = {0};

    for (i = 0; i < 4; ++i)
    {
        // create parent -> child pipe
        if (pipe(fdp2c[i]) < 0)
        {
            printf("Error in creating parent -> child pipe\n");
            exit(1);
        }
        // create child -> parent pipe
        if (pipe(fdc2p[i]) < 0)
        {
            printf("Error in creating child -> parent pipe\n");
            exit(1);
        }
    }
    if (fork() == 0)
    { // FCFS
        close(fdp2c[0][1]); // close fd[1] (output) in child side for parent -> child
        close(fdc2p[0][0]); // close fd[0] (input) in child side for child -> parent
        for (i = 0; i < 4; i++)
        {
            if (i != 0)
            {
                // close pipes not related to this child
                close(fdp2c[i][0]);
                close(fdp2c[i][1]);
                close(fdc2p[i][0]);
                close(fdc2p[i][1]);
            }
        }
        FCFS();
        printf("This is child process for FCFS (finished)\n");
        close(fdp2c[0][0]);
        close(fdc2p[0][1]);
        exit(0);
    }

    // Test cases
    strcpy(cmmd[0], "1"); // create a team
    strcpy(cmmd[1], "Team_A");
    strcpy(cmmd[2], "Project_A");
    strcpy(cmmd[3], "Alan");
    strcpy(cmmd[4], "Cathy");
    strcpy(cmmd[5], "Fanny");
    strcpy(cmmd[6], "Helen");

    write(fdp2c[0][1], cmmd, 900 * sizeof(char));
    memset(cmmd,0, 900 * sizeof(char));

    strcpy(cmmd[0], "1"); // creat the 2nd team
    strcpy(cmmd[1], "Team_B");
    strcpy(cmmd[2], "Project_B");
    strcpy(cmmd[3], "Cathy");
    strcpy(cmmd[4], "Helen");

    write(fdp2c[0][1], cmmd, 900 * sizeof(char));
    memset(cmmd,0, 900 * sizeof(char));

    strcpy(cmmd[0], "2a"); // make a booking
    strcpy(cmmd[1], "Team_A");
    strcpy(cmmd[2], "2022-04-25");
    strcpy(cmmd[3], "15:00");
    strcpy(cmmd[4], "2");

    write(fdp2c[0][1], cmmd, 900 * sizeof(char));
    memset(cmmd,0, 900 * sizeof(char));

    strcpy(cmmd[0], "2a");
    strcpy(cmmd[1], "Team_A");
    strcpy(cmmd[2], "2022-04-25");
    strcpy(cmmd[3], "12:00");
    strcpy(cmmd[4], "2");

    write(fdp2c[0][1], cmmd, 900 * sizeof(char));
    memset(cmmd,0, 900 * sizeof(char));

    strcpy(cmmd[0], "2a");
    strcpy(cmmd[1], "Team_A");
    strcpy(cmmd[2], "2022-04-25");
    strcpy(cmmd[3], "09:00");
    strcpy(cmmd[4], "1");

    write(fdp2c[0][1], cmmd, 900 * sizeof(char));
    memset(cmmd,0, 900 * sizeof(char));

    strcpy(cmmd[0], "2a");
    strcpy(cmmd[1], "Team_A");
    strcpy(cmmd[2], "2022-04-25");
    strcpy(cmmd[3], "14:00");
    strcpy(cmmd[4], "2");

    write(fdp2c[0][1], cmmd, 900 * sizeof(char));
    memset(cmmd,0, 900 * sizeof(char));

    strcpy(cmmd[0], "2a");
    strcpy(cmmd[1], "Team_A");
    strcpy(cmmd[2], "2022-04-25");
    strcpy(cmmd[3], "11:00");
    strcpy(cmmd[4], "2");

    write(fdp2c[0][1], cmmd, 900 * sizeof(char));
    memset(cmmd,0, 900 * sizeof(char));

    strcpy(cmmd[0], "2a"); // make a booking for team_B, should be time-clashed with the previous booking of team_A
    strcpy(cmmd[1], "Team_B");
    strcpy(cmmd[2], "2022-04-25");
    strcpy(cmmd[3], "14:00");
    strcpy(cmmd[4], "2");

    write(fdp2c[0][1], cmmd, 900 * sizeof(char));
    memset(cmmd,0, 900 * sizeof(char));

    strcpy(cmmd[0], "3"); // print schedule from 4-25 to 4-27
    strcpy(cmmd[1], "FCFS");
    strcpy(cmmd[2], "25");
    strcpy(cmmd[3], "27");

    write(fdp2c[0][1], cmmd, 900 * sizeof(char));
    memset(cmmd,0, 900 * sizeof(char));

    strcpy(cmmd[0], "end");

    write(fdp2c[0][1], cmmd, 900 * sizeof(char));
    memset(cmmd,0, 900 * sizeof(char));

    wait(NULL);
    exit(0);
}
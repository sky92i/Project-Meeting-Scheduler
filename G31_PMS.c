#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#define NUM_STAFF 8 // number of staff
#define NUM_APR_DATE 6 // number of dates in April
#define NUM_MAY_DATE 12 // number of dates in May
#define WORK_HR 9 // working hours per day
#define MAX_MEETING_HR 5 // humane criteria

// objects
typedef struct {
    char team[21];
    char project[21];
    int managerIndex; // initialized as boolean value (0/1) to check if isManager, can change to store name of project if needed
    int memberAIndex;
    int memberBIndex;
    int memberCIndex;
    //int availability[162] ;
}team;

typedef struct {
    int teamsIndex;
    int date;
    int startTime;
    int duration;
}meeting;

meeting recvMeetings[600] = {0}; // in child process, this array is used to receive all meeting requests (raw data)
meeting meetings[600] = {0}; // in child process, this array is used to handle scheduling
team teams[20] = {0};
int fdp2c[1][2]; // parent -> child pipes
int fdc2p[1][2]; // child -> parent pipes
char *staff[] = {"Alan", "Billy", "Cathy", "David", "Eva", "Fanny", "Gary", "Helen"};
int managerCount[NUM_STAFF] = {0};
int memberCount[NUM_STAFF] = {0};
int staffCount = NUM_STAFF, slotsCount = 0, meetingsCount = 0, teamsCount = 0;
int validDates[] = {25, 26, 27, 28, 29, 30, 2, 3, 4, 5, 6, 7, 9, 10, 11, 12, 13, 14};
// only the meetings between 25/4 to 14/5 will be considered valid
int fcfs = 1; int prio = 1; // for file export counting

int numberOfRejects = 0;
meeting acceptedMeetingsApr[NUM_STAFF][300] = {0}; // "8" stands for the number of staff members
meeting acceptedMeetingsMay[NUM_STAFF][300] = {0}; // same as the above, not yet sorted by date and start time at the very beginning
meeting acceptedMeetingsInorder[NUM_STAFF][600] = {0}; // combined of the meetings in both months, sorted by date and start time
int numAcceptedMeetingsApr[NUM_STAFF] = {0}; // number of meetings accepted for each member
int numAcceptedMeetingsMay[NUM_STAFF] = {0}; // children
int rejectedMeetingIndex[200] = {-1}; // children
meeting rejectedMeetingsApr[300] = {0}; // children
meeting rejectedMeetingsMay[300] = {0}; // children
meeting rejectedMeetings[600] = {0}; // children
int numRejectedMeetingsApr = 0; // children
int numRejectedMeetingsMay = 0; // children
int apr[NUM_APR_DATE][NUM_STAFF][WORK_HR] = {0}; // totally 6 working days in April, 8 members, 9 working hours per day
int may[NUM_MAY_DATE][NUM_STAFF][WORK_HR] = {0}; // totally 12 working days in May
// the first element ([X][X][0]) of the staff on the day stands for the time 0900-1000, etc. The last element ([X][X][9]) stands for the time 1700-1800
// considered boolean type, 0 stands for available, 1 stands for n/a
int meetingHoursApr[NUM_APR_DATE][NUM_STAFF] = {0}; // humane criteria, recording the meeting hours of each member each day, 6 days in April
int meetingHoursMay[NUM_MAY_DATE][NUM_STAFF] = {0}; // 12 days in May

// constructors
int creatTeam(team teams[], int index, int managerCount[], int memberCount[], char *team, char *project, int managerIndex, int memberAIndex, int memberBIndex, int memberCIndex){
    if (managerCount[managerIndex] == 1) return -1; // manager is already another project's manager
    if (memberAIndex != -1){if(memberCount[memberAIndex] >= 3) return -2;}
    if (memberBIndex != -1){if(memberCount[memberBIndex] >= 3) return -2;}
    if (memberCIndex != -1){if(memberCount[memberCIndex] >= 3) return -2;}
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

int createMeeting(meeting meetings[], int meetingsCount, team teams[], int teamsCount, char *teamName, char *date, char *startTime, char *duration){
    int i;
    char tmp[3] = {0};
    for (i = 0; i < teamsCount; i++){
        if (strcmp(teams[i].team, teamName) == 0){
            meetings[meetingsCount].teamsIndex = i;
        }
    }
    tmp[0] = date[8]; tmp[1] = date[9]; tmp[2] = 0; // find days in date e.g. 25 in 2022-04-25
    meetings[meetingsCount].date = atoi(tmp);
    tmp[0] = startTime[0]; tmp[1] = startTime[1]; // find hours in startTime e.g. 09 in 09:00
    meetings[meetingsCount].startTime = atoi(startTime);
    meetings[meetingsCount].duration = atoi(duration);
    return 1;
}

int searchStaffIndex(char *input)
{
    int i;
    for (i = 0; i < staffCount; i++)
    {
        if (strcmp(staff[i], input) == 0) return i;
    }
    return -1;
}

void clearSchedule() // should be called by children after printing schedule every time, avoid mixing the data with next schedule command
{
    numberOfRejects = 0;
    memset(acceptedMeetingsApr, 0, 800 * sizeof(meeting));
    memset(acceptedMeetingsMay, 0, 800 * sizeof(meeting));
    memset(acceptedMeetingsInorder, 0, 1600 * sizeof(meeting));
    memset(numAcceptedMeetingsApr, 0, 8 * sizeof(int));
    memset(numAcceptedMeetingsMay, 0, 8 * sizeof(int));
    memset(rejectedMeetingIndex, 0, 200 * sizeof(int));
    memset(rejectedMeetingsApr, 0, 100 * sizeof(meeting));
    memset(rejectedMeetingsMay, 0, 100 * sizeof(meeting));
    memset(rejectedMeetings, 0, 200 * sizeof(meeting));
    numRejectedMeetingsApr = 0;
    numRejectedMeetingsMay = 0;
    memset(apr, 0, NUM_APR_DATE * NUM_STAFF * WORK_HR *sizeof(int));
    memset(may, 0, NUM_MAY_DATE * NUM_STAFF * WORK_HR * sizeof(int));
    memset(meetingHoursApr, 0, NUM_APR_DATE * NUM_STAFF * sizeof(int));
    memset(meetingHoursMay, 0, NUM_MAY_DATE * NUM_STAFF * sizeof(int));
}

void sortMeetings(meeting unsortedMeetings[], int numberOfMeetings) // called by children
{
    int minIndex, i, j;
    for (i = 0; i < numberOfMeetings - 1; i++)
    {
        minIndex = i;
        for (j = i + 1; j < numberOfMeetings; j++)
        {
            if (    (unsortedMeetings[j].date < unsortedMeetings[minIndex].date) || // if the date of the first meeting is less than the second one's
                    (unsortedMeetings[j].date == unsortedMeetings[minIndex].date && unsortedMeetings[j].startTime < unsortedMeetings[minIndex].startTime)) // if they are on the same day, compare their start time
            {
                minIndex = j;
            }
        }
        if (minIndex != i)
        {
            // swapping of two meetings
            meeting dummy;
            memcpy(&dummy, &unsortedMeetings[minIndex], sizeof(meeting));
            memcpy(&unsortedMeetings[minIndex], &unsortedMeetings[i], sizeof(meeting));
            memcpy(&unsortedMeetings[i], &dummy, sizeof(meeting));
        }
    }
}

void mergeAprMay(meeting mergedMeetings[], meeting sortedMeetingsInApr[], meeting sortedMeetingsInMay[], int numberOfMeetingsInApr, int numberOfMeetingsInMay) // called by children
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

void prioSortMeetings(meeting original[], meeting prioSortedMeetings[])
// priority sorting, the meetings of the team with the smaller alphabet as the team name have higher priority, e.g. the meetings of Team_A should always have higher priority than that of Team_B
// assume that all the team names are in the format of "Team_*", and "*" is the single alphabet in uppercase
// stable sorting
// called by children
{
    memcpy(prioSortedMeetings, original, sizeof(meetings));
    int minIndex, i, j;
    for (i = 0; i < meetingsCount - 1; i++)
    {
        minIndex = i;
        for (j = i + 1; j < meetingsCount; j++)
        {
            char alphabet1 = teams[prioSortedMeetings[minIndex].teamsIndex].team[5];
            char alphabet2 = teams[prioSortedMeetings[j].teamsIndex].team[5];
            if(alphabet1 > alphabet2) // in ASCII, e.g., 'A' < 'B', but 'A' should have higher priority
            {
                minIndex = j;
            }
        }
        if (minIndex != i)
        {
            // swap
            meeting dummy;
            memcpy(&dummy, &prioSortedMeetings[minIndex], sizeof(meeting));
            memcpy(&prioSortedMeetings[minIndex], &prioSortedMeetings[i], sizeof(meeting));
            memcpy(&prioSortedMeetings[i], &dummy, sizeof(meeting));
        }
    }
}

// check request validity
int checkRequest(int teamsCount, team teams[], char input1[], char input2[], char input3[], char input4[]){
    //printf("%d\n", teamsCount);
    //printf("%s %s %s %s\n", input1, input2, input3, input4);

    // check team existence
    int exist = 0;
    int x;
    for (x = 0; x < teamsCount; x++){
        //printf("%s %s\n", &teams[x].team[5], &input1[5]);
        if (strcmp(&teams[x].team[5], &input1[5]) == 0 && &teams[x].team[5] != 0){
            exist = 1;
            break;
        }
    }
    if(exist != 1) {
        printf("[x] Entered team does not exist.\n");
        return -1;
    }

    // check date
    char april[] = "2022-04-";
    char may[] = "2022-05-";
    if( (strncmp(input2, april, 8) == 0 && (atoi(&input2[8]) < 25 || atoi(&input2[8]) > 31)) ||
        (strncmp(input2, may, 8) == 0 && (atoi(&input2[8]) < 1 || atoi(&input2[8]) > 14))  ||
        (strncmp(input2, april, 8) != 0 && strncmp(input2, may, 8) != 0) ) {
        printf("[x] Invalid date. The valid meeting date is from 2022-04-25 to 2022-05-14.\n");
        return -1;
    }
    if(atoi(&input2[8]) == 1 || atoi(&input2[8]) == 8) {
        printf("[x] Invalid date. Sunday is not a working day.\n");
        return -1;
    }

    // check time
    if (atoi(&input3[0]) < 9 || atoi(&input3[0]) > 18){
        printf("[x] Invalid time. The valid meeting time is from 09:00 to 18:00.\n");
        return -1;
    }

    // check duration
    if (atoi(&input4[0]) < 1 || atoi(&input4[0]) > 9){
        printf("[x] Invalid duration. Duration cannot < 1 or > 9.\n");
        return -1;
    }
    if ((atoi(&input4[0]) + atoi(&input3[0])) > 18 ){
        printf("[x] Invalid duration. The end time is after 18:00.\n");
        return -1;
    }
    return 0;
}

int searchValidDataIndex(int validDate)
{
    int i;
    for (i = 0; i < NUM_APR_DATE+NUM_MAY_DATE; i++)
    {
        if (validDate == validDates[i])
        {
            return i;
        }
    }
    return -1; // the date is not valid
}

int printScheduleCmmdCheck(char startDate[], char endDate[]) // to ensure the start date is less than the end date, if else reject the command
{
    int start = atoi(startDate); // convert to int
    int end;
    if (endDate[0] == '0') // convert to int
    {
        end = endDate[1] - '0';
    }
    else
    {
        end = atoi(endDate);
    }
    int startIndex = searchValidDataIndex(start);
    int endIndex = searchValidDataIndex(end);
    if (startIndex == -1)
    {
        printf("[x] The start date is not valid.\n");
        return 0;
    }
    if (endIndex == -1)
    {
        printf("[x] The end date is not valid.\n");
        return 0;
    }
    if (startIndex > endIndex)
    {
        printf("[x] The start date cannot be greater than the end date. Schedule request rejected.\n");
        return 0;
    }
    return 1;
}

void scheduleAndPrint() // only to be called by children
{
    char recvCmmd[30][30]; // received command

    while (1) // until a quit message is received from the parent
    {
        read(fdp2c[0][0], recvCmmd, 900 * sizeof(char));
        /*printf("(Child) Received from parent: %s, %s, %s, %s, %s, %s, %s\n", recvCmmd[0], recvCmmd[1], recvCmmd[2],
               recvCmmd[3], recvCmmd[4], recvCmmd[5], recvCmmd[6]); // for debugging, showing what have been received*/
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
            createMeeting(recvMeetings, meetingsCount, teams, teamsCount, recvCmmd[1], recvCmmd[2], recvCmmd[3],
                          recvCmmd[4]);
            meetingsCount++;
        }
        else if (strcmp(recvCmmd[0], "3") == 0) // print meeting schedule
            // recvCmmd[0] = "3"
            // recvCmmd[1] = "FCFS" or "PRIO"
            // recvCmmd[2] = "start date"
            // recvCmmd[3] = "end date"
        {
            FILE *file;
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
            char stringStartDate[4];
            char stringEndDate[4];
            memset(stringStartDate, 0, 4 * sizeof(char));
            memset(stringEndDate, 0, 4 * sizeof(char)); // clearing string of the last processing, avoid confusing
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
            if (startDateIndex > endDateIndex)
            {
                printf("Error: the start date is greater than the end date\n");
                continue;
                // should not be run
            }
            if (strcmp(recvCmmd[1], "PRIO") == 0) // if the selected algo is PRIO
            {
                prioSortMeetings(recvMeetings, meetings);
                char fname[] = "G31_PRIO_Schd_";     //deal with the file name
                char buf[5];
                sprintf(buf, "%d", prio);
                if (prio < 10){ strcat(fname, "0"); }
                strcat(fname, buf);
                prio++;
                strcat(fname, ".dat");
                file  = fopen(fname, "w");     //create a file with name
                system("clear");
                printf(">>>>>> Printed. Export file name: %s.\n\n", fname);
            }
            else if (strcmp(recvCmmd[1], "FCFS") == 0)
            {
                memcpy(meetings, recvMeetings, sizeof(recvMeetings));
                char fname[] = "G31_FCFS_Schd_";     //deal with the file name
                char buf[5];
                sprintf(buf, "%d", fcfs);
                if (fcfs < 10){ strcat(fname, "0"); }
                strcat(fname, buf);
                fcfs++;
                strcat(fname, ".dat");
                file  = fopen(fname, "w");     //create a file with name
                system("clear");
                printf(">>>>>> Printed. Export file name: %s.\n\n", fname);
            }

            for (i = 0; i < meetingsCount; i++)
            {
                int j, k;
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
                    int timeIndex = startTime - WORK_HR; // offset: 0900 --> 0, 1000 --> 1, ... , 1700 --> 8. Remarks: its not allowed that the start time to be 1800
                    for (k = 0; k < NUM_STAFF; k++) // 8 staff members, checking human criteria: meeting hours not exceeding 5 per day
                    {
                        if ((duration + meetingHoursApr[dateIndex][k]) > MAX_MEETING_HR)
                        {
                            accepted = 0;
                            break;
                        }
                    }
                    if (accepted == 0)
                    {
                        rejectedMeetingIndex[numberOfRejects++] = i;
                        continue; // go to check next booked meeting
                    }
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
                        memcpy(&acceptedMeetingsApr[teams[meetings[i].teamsIndex].managerIndex][numAcceptedMeetingsApr[teams[meetings[i].teamsIndex].managerIndex]++], &meetings[i],sizeof(meeting)); // a meeting is accepted, update the related record for the manager
                        meetingHoursApr[dateIndex][teams[meetings[i].teamsIndex].managerIndex] += duration; // meeting hours of the manager on that day increased
                        if (memberA != -1) // record-updating
                        {
                            memcpy(&acceptedMeetingsApr[memberA][numAcceptedMeetingsApr[memberA]++], &meetings[i],
                                   sizeof(meeting));
                            meetingHoursApr[dateIndex][memberA] += duration;
                        }
                        if (memberB != -1) // record-updating
                        {
                            memcpy(&acceptedMeetingsApr[memberB][numAcceptedMeetingsApr[memberB]++], &meetings[i],
                                   sizeof(meeting));
                            meetingHoursApr[dateIndex][memberB] += duration;
                        }
                        if (memberC != -1) // record-updating
                        {
                            memcpy(&acceptedMeetingsApr[memberC][numAcceptedMeetingsApr[memberC]++], &meetings[i],
                                   sizeof(meeting));
                            meetingHoursApr[dateIndex][memberC] += duration;
                        }
                        for (j = 0; j < duration; j++) // updating the time slots for each member
                        {
                            apr[dateIndex][teams[meetings[i].teamsIndex].managerIndex][timeIndex +
                                                                                       j] = 1; // occupy the timeslot, manager
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
                    for (k = 0; k < NUM_STAFF; k++) // 8 staff members, checking human criteria: meeting hours not exceeding 5 per day
                    {
                        if ((duration + meetingHoursMay[dateIndex][k]) > MAX_MEETING_HR) // exceeding 5 hours
                        {
                            accepted = 0;
                            break;
                        }
                    }
                    if (accepted == 0)
                    {
                        rejectedMeetingIndex[numberOfRejects++] = i;
                        continue; // go to check next booked meeting
                    }
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
                        memcpy(&acceptedMeetingsMay[teams[meetings[i].teamsIndex].managerIndex][numAcceptedMeetingsMay[teams[meetings[i].teamsIndex].managerIndex]++], &meetings[i],sizeof(meeting)); // a meeting is accepted, update the related record for the manager
                        meetingHoursMay[dateIndex][teams[meetings[i].teamsIndex].managerIndex] += duration;
                        if (memberA != -1) // record-updating
                        {
                            memcpy(&acceptedMeetingsMay[memberA][numAcceptedMeetingsMay[memberA]++], &meetings[i],
                                   sizeof(meeting));
                            meetingHoursMay[dateIndex][memberA] += duration;
                        }
                        if (memberB != -1) // record-updating
                        {
                            memcpy(&acceptedMeetingsMay[memberB][numAcceptedMeetingsMay[memberB]++], &meetings[i],
                                   sizeof(meeting));
                            meetingHoursMay[dateIndex][memberB] += duration;
                        }
                        if (memberC != -1) // record-updating
                        {
                            memcpy(&acceptedMeetingsMay[memberC][numAcceptedMeetingsMay[memberC]++], &meetings[i],
                                   sizeof(meeting));
                            meetingHoursMay[dateIndex][memberC] += duration;
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
            for (i = 0; i < NUM_STAFF; i++) // 8 staff members in total
            {
                sortMeetings(acceptedMeetingsApr[i],numAcceptedMeetingsApr[i]); // meetings in April
                sortMeetings(acceptedMeetingsMay[i],numAcceptedMeetingsMay[i]); // meetings in May
                mergeAprMay(acceptedMeetingsInorder[i], acceptedMeetingsApr[i], acceptedMeetingsMay[i], numAcceptedMeetingsApr[i], numAcceptedMeetingsMay[i]);
            }
            // sort rejected meetings, similar to the above
            for (i = 0; i < numberOfRejects; i++)
            {
                int rejectedMeetingDate = meetings[rejectedMeetingIndex[i]].date;
                if (rejectedMeetingDate >= 25 && rejectedMeetingDate <= 30) // the rejected meetings in April
                {
                    memcpy(&rejectedMeetingsApr[numRejectedMeetingsApr++], &meetings[rejectedMeetingIndex[i]], sizeof(meeting));
                }
                else if (rejectedMeetingDate >= 2 && rejectedMeetingDate <= 14) // the rejected meetings in May
                {
                    memcpy(&rejectedMeetingsMay[numRejectedMeetingsMay++], &meetings[rejectedMeetingIndex[i]], sizeof(meeting));
                }
            }
            sortMeetings(rejectedMeetingsApr, numRejectedMeetingsApr);
            sortMeetings(rejectedMeetingsMay, numRejectedMeetingsMay);
            mergeAprMay(rejectedMeetings, rejectedMeetingsApr, rejectedMeetingsMay, numRejectedMeetingsApr, numRejectedMeetingsMay);
            // TODO: print the schedule to a txt file
            // printing on terminal, for testing, temporary codes
            //printf("*** Project Meeting ***\n\n");
            fprintf(file, "*** Project Meeting ***\n\n");
            //printf("Algorithm used: %s\n", recvCmmd[1]);
            fprintf(file, "Algorithm used: %s\n", recvCmmd[1]);
            int startMonth;
            int endMonth;
            if (startDate >= 25 && startDate <= 30)
            {
                startMonth = 4;
                sprintf(stringStartDate, "%d", startDate);
            }
            else
            {
                startMonth = 5;
                if (startDate >= 2 && startDate <= 9)
                {
                    strcpy(stringStartDate, "0");
                }
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
                if (endDate >= 2 && endDate <= 9)
                {
                    strcpy(stringEndDate, "0");
                }
                char dummStr[3];
                sprintf(dummStr, "%d", endDate);
                strcat(stringEndDate, dummStr);
            }
            //printf("Period: 2022-0%d-%s to 2022-0%d-%s\n", startMonth, stringStartDate, endMonth, stringEndDate);
            fprintf(file, "Period: 2022-0%d-%s to 2022-0%d-%s\n", startMonth, stringStartDate, endMonth, stringEndDate);
            for (i = 0; i < 8; i++) // 8 staff members
            {
                int j;
                //printf("Date\t\t\tStart\t\tEnd\t\tTeam\t\tProject\n");
                fprintf(file, "Date\t\t\tStart\t\tEnd\t\tTeam\t\tProject\n");
                //printf("===============================================================================================\n");
                fprintf(file, "===============================================================================================\n");
                for (j = startDateIndex; j < endDateIndex+1; j++)
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
                            if (currentDate >= 2 && currentDate <= 9)
                            {
                                strcat(dummyDate, "0");
                            }
                        }
                        sprintf(dummyDate2, "%d", currentDate);
                        strcat(dummyDate, dummyDate2);
                        strcat(stringDate, dummyDate); // string date ok, to be printed
                        if (currentStart == 9)
                        {
                            strcpy(stringStartTime, "0");
                        }
                        sprintf(dummyTime, "%d", currentStart);
                        strcat(stringStartTime, dummyTime);
                        strcat(stringStartTime, ":00"); // string start time ok, to be printed
                        sprintf(dummyTime, "%d", currentEnd);
                        strcat(stringEndTime, dummyTime);
                        strcat(stringEndTime, ":00"); // string end time ok, to be printed
                        //printf("%s\t\t%s\t\t%s\t\t%s\t\t%s\n", stringDate, stringStartTime, stringEndTime, currentTeamName, currentProject);
                        fprintf(file, "%s\t\t%s\t\t%s\t\t%s\t\t%s\n", stringDate, stringStartTime, stringEndTime, currentTeamName, currentProject);
                    }
                }
                //printf("===============================================================================================\n");
                fprintf(file, "===============================================================================================\n");
                //printf("Staff: %s\n\n\n",staff[i]);
                fprintf(file, "Staff: %s\n\n\n",staff[i]);
            }
            //printf("\t\t\t\t\t\t\t\t\t- End -\n");
            fprintf(file, "\t\t\t\t\t\t\t\t\t- End -\n");
            // rejected meetings
            //printf("\n\n*** Meeting Request - REJECTED ***\n\n");
            fprintf(file, "\n\n*** Meeting Request - REJECTED ***\n\n");
            //printf("===============================================================================================\n");
            fprintf(file, "===============================================================================================\n");
            int a = 0;
            for (i = startDateIndex; i < endDateIndex+1; i++)
            {
                int j;
                for (j = 0; j < numberOfRejects; j++)
                {
                    int currentDate = rejectedMeetings[j].date;
                    int currentStart = rejectedMeetings[j].startTime;
                    int currentTeamIndex = rejectedMeetings[j].teamsIndex;
                    if (currentDate != validDates[i])
                    {
                        continue;
                    }
                    char currentTeamName[21]; // to be printed
                    char stringDate[21] = "2022-"; // to be printed
                    char dummyDate[10], dummyDate2[4];
                    char stringStartTime[10] = {0};
                    char dummyTime[6];
                    strcpy(currentTeamName, teams[currentTeamIndex].team);
                    if (currentDate >= 25 && currentDate <= 30) // april
                    {
                        strcpy(dummyDate, "04-");
                    }
                    else // may
                    {
                        strcpy(dummyDate, "05-");
                        if (currentDate >= 2 && currentDate <= 9) {
                            strcat(dummyDate, "0");
                        }
                    }
                    sprintf(dummyDate2, "%d", currentDate);
                    strcat(dummyDate, dummyDate2);
                    strcat(stringDate, dummyDate); // string date ok, to be printed
                    if (currentStart == 9)
                    {
                        strcpy(stringStartTime, "0");
                    }
                    sprintf(dummyTime, "%d", currentStart);
                    strcat(stringStartTime, dummyTime);
                    strcat(stringStartTime, ":00"); // string start time ok, to be printed
                    //printf("%d.\t%s %s %s %d\n", j+1, currentTeamName, stringDate, stringStartTime, rejectedMeetings[j].duration);
                    fprintf(file, "%d.\t%s %s %s %d\n", j+1, currentTeamName, stringDate, stringStartTime, rejectedMeetings[j].duration);
                    a++;
                }
            }
            //printf("===============================================================================================\n");
            fprintf(file, "===============================================================================================\n");
            //printf("There are %d requests rejected for the required period.\n\n\n", a);
            fprintf(file, "There are %d requests rejected for the required period.\n\n\n", a);
            //printf("\t\t\t\t\t\t\t\t\t- End -\n");
            fprintf(file, "\t\t\t\t\t\t\t\t\t- End -\n");
            fclose(file);
            clearSchedule();
        }
        else if (strcmp(recvCmmd[0], "end") == 0)
        {
            return;
        }
    }
}


int main(int argc, char *argv[]){
    // variables
    int i, j, valid;
    char batchBuf[80];
    char tmpDate[3];
    char tmpInput[80] = {0};
    char input[30][30] = {0};
    char command[30][30] = {0};
    FILE *log = NULL;
    FILE *batchFile = NULL;
    int option = 0;
    int staffAindex, staffBindex, staffCindex, staffDindex;

    // create pipes for 1 child process
    for (i = 0; i < 1; ++i) {
        // create parent -> child pipe
        if (pipe(fdp2c[i]) < 0){
            printf("Error in creating parent -> child pipe\n");
            exit(1);
        }
        // create child -> parent pipe
        if (pipe(fdc2p[i]) < 0){
            printf("Error in creating child -> parent pipe\n");
            exit(1);
        }
    }

    if (fork() == 0){ // Scheduling process
        close(fdp2c[0][1]); // close fd[1] (output) in child side for parent -> child
        close(fdc2p[0][0]); // close fd[0] (input) in child side for child -> parent
        for (i = 0; i < 4 ; i++) {
            if (i != 0) {
                // close pipes not related to this child
                close(fdp2c[i][0]);
                close(fdp2c[i][1]);
                close(fdc2p[i][0]);
                close(fdc2p[i][1]);
            }
        }
        scheduleAndPrint();
        // printf("Child process finished.\n");
        close(fdp2c[0][0]);
        close(fdc2p[0][1]);
        exit(0);
    }

    // parent
    sleep(1); // testing

    // closed unused pipes for parent
    for (i = 0; i < 1; ++i) {
        close(fdp2c[i][0]);
        close(fdc2p[i][1]);
    }
    // input variables
    char *cmd1 = {"\n1.  Create Project Team\n"};
    char *cmd2 = {"\n2.  Project Meeting Request\n"
                  "    2a.  Single input\n"
                  "    2b.  Batch input\n"};
    //"    2c.  Meeting Attendance\n\n"};               (if implemented)
    char *cmd3 = {"\n3.  Print Meeting Schedule\n"
                  "    3a. FCFS (First Come First Served)\n"
                  "    3b. PRIO (Priority)\n"}; // TODO change XXXX
    //"    3c. YYYY (Attendance Report)\n\n"};           (if implemented)
    char *cmd4 = {"\n4.  Exit\n"};
    char* prompt = {"\nEnter an option: "};

    // log file
    log = fopen("record.log", "w");
    if (log == NULL){
        printf("Error in opening log file\n");
        exit(1);
    }

    // user menu
    do {
        // 2 level user interface
        system("clear");
        printf("\n   ~~ WELCOME TO PolyStar ~~\n");
        printf("%s", cmd1);
        printf("%s", cmd2);
        printf("%s", cmd3);
        printf("%s", cmd4);
        printf("%s", prompt);

        // user inputs 1 / 2 / 3 / 4 for option
        fgets(tmpInput, 80, stdin);
        option = atoi(tmpInput);

        int count = 0;
        char *tmp = NULL;

        int printedCMD = 0;

        // Menu part 1 for option 1
        while(option == 1){
            if(printedCMD == 0) {
                system("clear");
                printf("%s", cmd1);
            }
            printedCMD = 1;
            printf("Enter> ");

            // separate input by space
            fgets(tmpInput, 80, stdin);
            tmpInput[strlen(tmpInput)-1] = 0; // remove '\n' character
            count = 0;
            tmp = strtok(tmpInput, " ");
            while (tmp != NULL){
                strncpy(command[count+1], tmp, 30);
                strncpy(input[count++], tmp, 30);
                tmp = strtok(NULL, " ");
            }

            if (strlen(input[0]) <= 1) { // if user inputs 0 to return main menu
                option = atoi(input[0]);
                continue;
            }
            else {
                staffAindex = 0, staffBindex = 0, staffCindex = 0, staffDindex = 0;
                staffAindex = searchStaffIndex(input[2]);
                staffBindex = searchStaffIndex(input[3]);
                staffCindex = searchStaffIndex(input[4]);
                staffDindex = searchStaffIndex(input[5]);
                valid = creatTeam(teams, teamsCount, managerCount, memberCount, input[0], input[1], staffAindex, staffBindex, staffCindex, staffDindex);
                //printf("hi,%s %s %d %d\n", teams[teamsCount].team, teams[teamsCount].project, teams[teamsCount].managerIndex, teams[teamsCount].memberBIndex);

                if (valid == 1) {
                    // sends command in array to scheduling process; in form:"1 Team_A Project_A Alan Cathy Fanny Helen"
                    strcpy(command[0], "1");
                    write(fdp2c[0][1], command, 900*sizeof(char));
                    printf(">>>>>> Project %s is created.\n\n", input[0]);
                    teamsCount++;
                }
                else if (valid == -1)
                    printf(">>>>>> Project %s is not created due to manager has exceed project limit.\n\n", input[0]);
                else
                    printf(">>>>>> Project %s is not created due to member has exceed project limit.\n\n", input[0]);
            }
            memset(input, 0, sizeof(input)); // clear previous input
            memset(command, 0, sizeof(command)); // clear previous command
        }


        // Menu part 2 for option 2
        /* inputs in form of "option_input"
         * e.g. "2a Team_A 2022-04-25 09:00 2" for 2a. Single input */
        while(option == 2){
            if(printedCMD == 0) {
                system("clear");
                printf("%s", cmd2);
            }
            printedCMD = 1;
            printf("Enter> ");

            // separate input by space
            fgets(tmpInput, 80, stdin);
            tmpInput[strlen(tmpInput)-1] = 0; // remove '\n' character
            count = 0;
            tmp = strtok(tmpInput, " ");
            while (tmp != NULL){
                strncpy(input[count++], tmp, 30);
                tmp = strtok(NULL, " ");
            }

            if (strlen(input[0]) <= 1) { // if user inputs 0 to return main menu
                option = atoi(input[0]);
                continue;
            }
            else {
                if (input[0][1] == 'a'){ // input for 2a. Single input
                    fprintf(log,"%s %s %s %s\n", input[1], input[2], input[3], input[4]);

                    // check the validity of the request.
                    if(checkRequest(teamsCount, teams, input[1], input[2], input[3], input[4]) == 0) {
                        // sends command in array to scheduling process; in form:"2a Team_A 2022-04-25 09:00 2"
                        write(fdp2c[0][1], input, 900*sizeof(char));
                        printf(">>>>>> Your request has been recorded.\n\n");
                    }
                    else printf(">>>>>> Your request is invalid.\n\n");
                }
                else{ // input for 2b. Batch input
                    // e.g. 2b batch01_Requests.dat for 2b input
                    batchFile = fopen(input[1],"r");
                    if (batchFile == NULL){
                        printf("Error in opening input file\n");
                        break;
                    }
                    int lineNum = 0;
                    while (fgets(batchBuf, 80, batchFile) != NULL){ // read batch input line by line
                        fputs(batchBuf, log); // write line from batch input to log

                        lineNum++;
                        char batchInput[30][30] = {0}; // for checkRequest function

                        // extract the info of a line and copy to batchInput
                        strncpy(batchInput[0], batchBuf, 6);
                        strncpy(batchInput[1], &batchBuf[7], 10);
                        strncpy(batchInput[2], &batchBuf[18], 5);
                        strncpy(batchInput[3], &batchBuf[24], 1);
                        printf("%s %s %s %s\n", batchInput[0], batchInput[1], batchInput[2], batchInput[3]);  // for debugging

                        // check the validity of the request. 0 is valid
                        if(checkRequest(teamsCount, teams, batchInput[0], batchInput[1], batchInput[2], batchInput[3]) == 0) {
                            printf(">>>>>> Line %d request has been recorded.\n\n", lineNum);
                            // stores command for algo process
                            strcpy(command[0], "2a");
                            strcpy(command[1], batchInput[0]);
                            strcpy(command[2], batchInput[1]);
                            strcpy(command[3], batchInput[2]);
                            strcpy(command[4], batchInput[3]);
                            // sends command in array to scheduling process; in form:"2a Team_A 2022-04-25 09:00 2"
                            write(fdp2c[0][1], command, 900*sizeof(char));
                        }
                        else printf(">>>>>> Line %d request is invalid.\n\n", lineNum);
                    }
                    fclose(batchFile);
                    printf(">>>>>> Your batch request has been recorded.\n\n");

                }
            }
            memset(input, 0, sizeof(input)); // clear previous input
            memset(command, 0, sizeof(command)); // clear previous command
        }

        // Menu part 3 for option 3
        /* inputs in form of "option_input"
         * e.g. "3a FCFS 2022-04-25 2022-04-27" for 3a. FCFS (First Come First Served) */
        while(option == 3){
            if(printedCMD == 0) {
                system("clear");
                printf("%s", cmd3);
                printf("Enter> ");
            }
            if(printedCMD == 1) {
                sleep(1);
                printf("%s", cmd3);
                printf("Enter> ");
            }
            printedCMD = 1;

            // separate input by space
            fgets(tmpInput, 80, stdin);
            tmpInput[strlen(tmpInput)-1] = 0; // remove '\n' character
            count = 0;
            tmp = strtok(tmpInput, " ");
            while (tmp != NULL){
                strncpy(input[count++], tmp, 30);
                tmp = strtok(NULL, " ");
            }

            if (strlen(input[0]) <= 1) { // if user inputs 0 to return main menu
                option = atoi(input[0]);
                continue;
            }
            else {
                strcpy(command[0], "3");
                tmpDate[0] = input[2][8]; tmpDate[1] = input[2][9]; tmpDate[2] = 0; // set for starting date
                strcpy(command[2], tmpDate);
                tmpDate[0] = input[3][8]; tmpDate[1] = input[3][9]; tmpDate[2] = 0; // set for ending date
                strcpy(command[3], tmpDate);
                strcpy(command[1],input[1]);
                // printf("date is %s, %s\n",command[2], command[3]);

                // sends command in array, e.g. input:"3a FCFS 2022-04-25 2022-04-27"; in form:"3 FCFS 25 27" to FCFS process
                // send command to scheduling process to print output
                if (printScheduleCmmdCheck(command[2], command[3]))
                {
                    write(fdp2c[0][1], command, 900 * sizeof(char));
                }
            }
            memset(input, 0, sizeof(input)); // clear previous input
            memset(command, 0, sizeof(command)); // clear previous command
        }

    } while (option != 4);
    fclose(log);

    strcpy(command[0], "end");
    write(fdp2c[0][1], command, 900 * sizeof(char)); // send end msg to the child
    wait(NULL); // wait the child
    close(fdp2c[0][1]);
    close(fdc2p[0][0]);
    printf("Exited successfully\n\n");
    return 0;
}


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

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

// not used
typedef struct {
    char name[21];
    int teamsIndex;
    int date;
    int startTime;
    int duration;
    int endTime;
}meeting;

// just an idea
typedef struct {
    int available; // initialized as boolean value (0/1) to check if available timeslot
    int teamsIndex; // index of the booked teams from teams array
    char *projectName[20];
}slot;

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

// an idea
meeting createMeeting(char name[21], int teamsIndex, int date, int startTime, int endTime){
    meeting a;
    strncpy(a.name, name, 21);
    a.teamsIndex = teamsIndex;
    a.date = date;
    a.startTime = startTime;
    a.endTime = endTime;
    a.duration = endTime - startTime;
    return a;
}

int searchStaffIndex(char *staff[], char *input, int staffCount){
    int i;
    for (i = 0; i < staffCount; i++){
        if (strcmp(staff[i], input) == 0) return i;
    }
    return -1;
}

char FilledSlot[1000][10];
int FilledSlotCount = 0;

int checkRequest(int teamsCount, team teams[], char input1[], char input2[], char input3[], char input4[]){
    int wrongInput = 0;
    int exist = 0;
    //printf("%d\n", teamsCount);
    //printf("%s %s %s %s\n", input1, input2, input3, input4);
    if (atoi(&input4[0]) < 1 || atoi(&input4[0]) > 9){
        printf("Invalid duration\n");
        wrongInput = 1;
        return 2;
    }
    for (int x = 0; x < teamsCount; x++){ // check team existence
        //printf("%s %s\n", &teams[x].team[5], &input1[5]);
        if (strcmp(&teams[x].team[5], &input1[5]) == 0 && &teams[x].team[5] != 0){
            exist = 1;
            break;
        }
    }
    if(exist != 1) {
        printf("Input team is not exist\n");
        return 2;
    }
    int i = 0;
    int timeSlotInvalid = 0;
    if(wrongInput != 1 && exist == 1) {
        for(i = 0; i < atoi(&input4[0]); i++) { // loop n times if duration is n
            char dateTimeTeam[10] = {0}; // used to save info of request, e.g. Team_A 2022-04-25 09:00 -> 2509A
            if(i >= 1) { // if duration is more than 1h
                char str1[10] = {0}; // temporary 
                sprintf(str1, "%d", atoi(&input3[0]) + i); // add duration e.g. 09+1=10 and then convert integer to string
                //printf("%s\n", str1);
                //printf("%s", &str1[0]);
                strcpy(&dateTimeTeam[0], &input2[8]); // e.g. Team_A 2022-04-25 09:00, dateTimeTeam is 25
                strcat(&dateTimeTeam[0], &str1[0]); // dateTimeTeam is 2509
                strcat(&dateTimeTeam[0], &input1[5]); // dateTimeTeam is 2509A
                //printf("%s", &dateTimeTeam[0]);
            }
            else{ // if duration is 1h only
                strcpy(&dateTimeTeam[0], &input2[8]);
                strncat(&dateTimeTeam[0], &input3[0], 2);
                strcat(&dateTimeTeam[0], &input1[5]);
            }

            for(int a = 0; a < FilledSlotCount; a++){
                if(strcmp(&dateTimeTeam[0], FilledSlot[a]) == 0) { // if same team in the same timeslot
                    printf("Time clash, same team in the same timeslot\n");
                    timeSlotInvalid = 1;
                }
                // if different team but same timeslot, need to check input team's member availabiliy.
                // i.e. Are there any common members in the current team and previously entered teams
                else if((strncmp(&dateTimeTeam[0], FilledSlot[a], 4) == 0) && (strcmp(&dateTimeTeam[0], FilledSlot[a]) != 0)) {
                    printf("The requested timeslot has another team requested before, further checking...\n");
                    int tmpTeamIndex1 = 0;
                    int tmpTeamIndex2 = 0;
                    for (int x = 0; x < teamsCount; x++){
                        if (strcmp(&teams[x].team[5], &FilledSlot[a][4]) == 0){ // find previously entered team index
                            tmpTeamIndex1 = x;
                            //printf("tmpIn1 %d\n", tmpTeamIndex1);
                        }
                        if (strcmp(&teams[x].team[5], &input1[5]) == 0){ // find current team index
                            tmpTeamIndex2 = x;
                            //printf("tmpIn2 %d\n", tmpTeamIndex2);
                        }
                    }

                    // previously entered team vs current team
                    if((teams[tmpTeamIndex1].managerIndex == teams[tmpTeamIndex2].memberAIndex) && teams[tmpTeamIndex1].managerIndex >= 0 && teams[tmpTeamIndex2].memberAIndex >= 0) {
                        printf("time clash1: Input team's member A is not avaliable\n"); timeSlotInvalid = 1;}
                    if((teams[tmpTeamIndex1].managerIndex == teams[tmpTeamIndex2].memberBIndex) && teams[tmpTeamIndex1].managerIndex >= 0 && teams[tmpTeamIndex2].memberBIndex >= 0) {
                        printf("time clash2: Input team's member B is not avaliable\n"); timeSlotInvalid = 1;}
                    if((teams[tmpTeamIndex1].managerIndex == teams[tmpTeamIndex2].memberCIndex) && teams[tmpTeamIndex1].managerIndex >= 0 && teams[tmpTeamIndex2].memberCIndex >= 0) {
                        printf("time clash3: Input team's member C is not avaliable\n"); timeSlotInvalid = 1;}

                    if((teams[tmpTeamIndex1].memberAIndex == teams[tmpTeamIndex2].managerIndex) && teams[tmpTeamIndex1].memberAIndex >= 0 && teams[tmpTeamIndex2].managerIndex >= 0) {
                        printf("time clash4: Input team's manager is not avaliable\n"); timeSlotInvalid = 1;}
                    if((teams[tmpTeamIndex1].memberAIndex == teams[tmpTeamIndex2].memberAIndex) && teams[tmpTeamIndex1].memberAIndex >= 0 && teams[tmpTeamIndex2].memberAIndex >= 0) {
                        printf("time clash5: Input team's member A is not avaliable\n"); timeSlotInvalid = 1;}
                    if((teams[tmpTeamIndex1].memberAIndex == teams[tmpTeamIndex2].memberBIndex) && teams[tmpTeamIndex1].memberAIndex >= 0 && teams[tmpTeamIndex2].memberBIndex >= 0) {
                        printf("time clash6: Input team's member B is not avaliable\n"); timeSlotInvalid = 1;}
                    if((teams[tmpTeamIndex1].memberAIndex == teams[tmpTeamIndex2].memberCIndex) && teams[tmpTeamIndex1].memberAIndex >= 0 && teams[tmpTeamIndex2].memberCIndex >= 0) {
                        printf("time clash7: Input team's member C is not avaliable\n"); timeSlotInvalid = 1;}

                    if((teams[tmpTeamIndex1].memberBIndex == teams[tmpTeamIndex2].managerIndex) && teams[tmpTeamIndex1].memberBIndex >= 0 && teams[tmpTeamIndex2].managerIndex >= 0) {
                        printf("time clash8: Input team's manager is not avaliable\n"); timeSlotInvalid = 1;}
                    if((teams[tmpTeamIndex1].memberBIndex == teams[tmpTeamIndex2].memberAIndex) && teams[tmpTeamIndex1].memberBIndex >= 0 && teams[tmpTeamIndex2].memberAIndex >= 0) {
                        printf("time clash9: Input team's member A is not avaliable\n"); timeSlotInvalid = 1;}
                    if((teams[tmpTeamIndex1].memberBIndex == teams[tmpTeamIndex2].memberBIndex) && teams[tmpTeamIndex1].memberBIndex >= 0 && teams[tmpTeamIndex2].memberBIndex >= 0) {
                        printf("time clash10: Input team's member B is not avaliable\n"); timeSlotInvalid = 1;}
                    if((teams[tmpTeamIndex1].memberBIndex == teams[tmpTeamIndex2].memberCIndex) && teams[tmpTeamIndex1].memberBIndex >= 0 && teams[tmpTeamIndex2].memberCIndex >= 0) {
                        printf("time clash11: Input team's member C is not avaliable\n"); timeSlotInvalid = 1;}

                    if((teams[tmpTeamIndex1].memberCIndex == teams[tmpTeamIndex2].managerIndex) && teams[tmpTeamIndex1].memberCIndex >= 0 && teams[tmpTeamIndex2].managerIndex >= 0) {
                        printf("time clash12: Input team's manager is not avaliable\n"); timeSlotInvalid = 1;}
                    if((teams[tmpTeamIndex1].memberCIndex == teams[tmpTeamIndex2].memberAIndex) && teams[tmpTeamIndex1].memberCIndex >= 0 && teams[tmpTeamIndex2].memberAIndex >= 0) {
                        printf("time clash13: Input team's member A is not avaliable\n"); timeSlotInvalid = 1;}
                    if((teams[tmpTeamIndex1].memberCIndex == teams[tmpTeamIndex2].memberBIndex) && teams[tmpTeamIndex1].memberCIndex >= 0 && teams[tmpTeamIndex2].memberBIndex >= 0) {
                        printf("time clash14: Input team's member B is not avaliable\n"); timeSlotInvalid = 1;}
                    if((teams[tmpTeamIndex1].memberCIndex == teams[tmpTeamIndex2].memberCIndex) && teams[tmpTeamIndex1].memberCIndex >= 0 && teams[tmpTeamIndex2].memberCIndex >= 0) {
                        printf("time clash15: Input team's member C is not avaliable\n"); timeSlotInvalid = 1;}
                }
            }

            if(timeSlotInvalid == 0) { // Record dateTimeTeam for valid request
                strcpy(FilledSlot[FilledSlotCount], &dateTimeTeam[0]);
                printf("slot filled: %s\n", FilledSlot[FilledSlotCount]); // for debugging
                FilledSlotCount++;
            }
            else break;
        }
    }
    return timeSlotInvalid;
}

int main(int argc, char *argv[]){
    // variables
    int i, j, valid;
    int fdp2c[4][2]; // parent -> child pipes
    int fdc2p[4][2]; // child -> parent pipes
    char buf1[80];
    char buf2[80];
    char batchBuf[80];
    int ppid = getpid(), pid = getpid();
    char tmpInput[80] = {0};
    char input[30][30] = {0};
    FILE *log = NULL;
    FILE *batchFile = NULL;
    int option = 0;
    int staffAindex, staffBindex, staffCindex, staffDindex;

    char *staff[] = {"Alan", "Billy", "Cathy", "David", "Eva", "Fanny", "Gary", "Helen"};
    int managerCount[8] = {0};
    int memberCount[8] = {0};
    slot slots[162] = {0};
    meeting meetings[200] = {0};
    team teams[20] = {0};
    int staffCount = 8, slotsCount =0 , meetingsCount = 0 , teamsCount = 0;

    // create pipes for 4 child process: FCFS, XXXX, rescheduling, output
    for (i = 0; i < 4; ++i) {
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

    // fork and close unused pipes
    for (i = 0; i < 4; ++i) {
        pid = fork();
        if (pid == 0){ // child
            for(j = 0; j < 4; j++){
                if (i == j) {
                    close(fdp2c[i][1]); // close fd[1] (output) in child side for parent -> child
                    close(fdc2p[i][0]); // close fd[0] (input) in child side for child -> parent
                }
                else{
                    // close pipes not related to this child
                    close(fdp2c[i][0]);
                    close(fdp2c[i][1]);
                    close(fdc2p[i][0]);
                    close(fdc2p[i][1]);
                }
            }
            break; // break so not to fork child of child
        }
    }
    pid = getpid();

    if (pid - ppid == 1){ // FCFS
        printf("This is child process for FCFS\n");
        close(fdp2c[0][0]);
        close(fdc2p[0][1]);
        exit(0);
    }
    else if (pid - ppid == 2){ // XXXX algorithm
        printf("This is child process for XXXX\n");
        close(fdp2c[1][0]);
        close(fdc2p[1][1]);
        exit(0);
    }
    else if (pid - ppid == 3){ // Rescheduling algorithm
        printf("This is child process for Rescheduling\n");
        close(fdp2c[2][0]);
        close(fdc2p[2][1]);
        exit(0);
    }
    else if (pid - ppid == 4){ // Output
        printf("This is child process for output\n");
        close(fdp2c[3][0]);
        close(fdc2p[3][1]);
        exit(0);
    }

    // parent

    sleep(1); // testing

    // closed unused pipes
    for (i = 0; i < 4; ++i) {
        close(fdp2c[i][0]);
        close(fdc2p[i][1]);
    }
    // input variables
    char *cmd1 = {"\n1.  Create Project Team\n"};
    char *cmd2 = {"\n2.  Project Meeting Request\n"
                  "    2a.  Single input\n"
                  "    2b.  Batch input\n"};
    //"    2c.  Meeting Attendance\n\n"};               (if implemented)
    char *cmd3 = {"\n3,  Print Meeting Schedule\n"
                  "    3a. FCFS (First Come First Served)\n"
                  "    3b. XXXX (Another algorithm implemented)\n"}; // TODO change XXXX
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

        // Menu part 1 for option 1
        while(option == 1){
            printf("%s", cmd1);
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

            if (strlen(input[0]) == 1) { // if user inputs 0 to return main menu
                option = atoi(input[0]);
            }
            else {
                staffAindex = 0, staffBindex = 0, staffCindex = 0, staffDindex = 0;
                staffAindex = searchStaffIndex(staff, input[2], staffCount);
                staffBindex = searchStaffIndex(staff, input[3], staffCount);
                staffCindex = searchStaffIndex(staff, input[4], staffCount);
                staffDindex = searchStaffIndex(staff, input[5], staffCount);
                valid = creatTeam(teams, teamsCount, managerCount, memberCount, input[0], input[1], staffAindex, staffBindex, staffCindex, staffDindex);
                //printf("hi,%s %s %d %d\n", teams[teamsCount].team, teams[teamsCount].project, teams[teamsCount].managerIndex, teams[teamsCount].memberBIndex);

                if (valid == 1) {
                    // TODO send command through pips to all child, but send what
                    printf(">>>>>> Project %s is created.\n", input[0]);
                    teamsCount++;
                }
                else if (valid == -1)
                    printf(">>>>>> Project %s is not created due to manager has exceed project limit.\n", input[0]);
                else
                    printf(">>>>>> Project %s is not created due to member has exceed project limit.\n", input[0]);
            }
            memset(input, 0, sizeof(input)); // clear previous input
        }


        // Menu part 2 for option 2
        /* inputs in form of "option_input"
         * e.g. "2a Team_A 2022-04-25 09:00 2" for 2a. Single input */
        while(option == 2){
            printf("%s", cmd2);
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

            if (strlen(input[0]) == 1) { // if user inputs 0 to return main menu
                option = atoi(input[0]);
            }
            else {
                if (input[0][1] == 'a'){ // input for 2a. Single input
                    fprintf(log,"%s %s %s %s\n", input[1], input[2], input[3], input[4]);
                    
                    // check the validity of the request.
                    if(checkRequest(teamsCount, teams, input[1], input[2], input[3], input[4]) == 0) {
                        printf("\n>>>>>> Your request has been accepted.\n");

//                      TODO
//                      Create meeting?
                    }
                    else if(checkRequest(teamsCount, teams, input[1], input[2], input[3], input[4]) == 1)
                        printf("\n>>>>>> Your request has been rejected.\n");
                    else if(checkRequest(teamsCount, teams, input[1], input[2], input[3], input[4]) == 2)
                        printf("\n>>>>>> Your request is invalid.\n");
                }
                else{ // input for 2b. Batch input
                    // e.g. 2b batch01_Requests.dat for 2b input
                    printf("hi,%s\n\n", input[1]);
                    batchFile = fopen(input[1],"r");
                    if (batchFile == NULL){
                        printf("Error in opening input file\n");
                        exit(1);
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
                            printf(">>>>>> Line %d request has been accepted.\n\n", lineNum);

//                      TODO
//                      Create meeting?
                        }
                        else if(checkRequest(teamsCount, teams, batchInput[0], batchInput[1], batchInput[2], batchInput[3]) == 1)
                            printf(">>>>>> Line %d request has been rejected.\n\n", lineNum);
                        else if(checkRequest(teamsCount, teams, batchInput[0], batchInput[1], batchInput[2], batchInput[3]) == 2)
                            printf(">>>>>> Line %d request is invalid.\n\n", lineNum);
                    }
                    fclose(batchFile);
                    printf(">>>>>> Your batch request has been recorded.\n");

                }
            }
            memset(input, 0, sizeof(input)); // clear previous input
        }

        // Menu part 3 for option 3
        /* inputs in form of "option_input"
         * e.g. "3a, FCFS 2022-04-25 2022-04-27" for 3a. FCFS (First Come First Served) */
        while(option == 3){
            printf("%s", cmd3);
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

            if (strlen(input[0]) == 1) { // if user inputs 0 to return main menu
                option = atoi(input[0]);
            }
            else {
                // TODO print schedule
                printf(">>>>>> Printed. Export file name: Schedule_FCFS_01.txt.\n");
            }
            memset(input, 0, sizeof(input)); // clear previous input
        }

    } while (option != 4);
    fclose(log);

    //printf("third is %s %s %d %d %d %d\n", teams[2].team, teams[2].project, teams[2].managerIndex, teams[2].memberAIndex,
    //       teams[2].memberBIndex, teams[2].memberCIndex);

    for (i = 0; i < 4; i ++){
        wait(NULL);
        close(fdp2c[i][1]);
        close(fdc2p[i][0]);
        //printf("Collected 1 child\n");
    }
    printf("Exited successfully\n\n");
    return 0;
}
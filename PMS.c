
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
                    // TODO check if valid then to print to user
//                    if (){
//                        printf(">>>>>> Your request has been accepted.\n");
//                    }
//                    else{
//                        printf(">>>>>> Your batch request has been rejected.\n");
//                    }
                }
                else{ // input for 2b. Batch input
                    printf("hi,%s\n", input[1]);
                    batchFile = fopen(input[1],"r");
                    if (batchFile == NULL){
                        printf("Error in opening input file\n");
                        exit(1);
                    }
                    while (fgets(batchBuf, 80, batchFile) != NULL){ // read batch input line by line
                        fputs(batchBuf, log); // write line from batch input to log
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
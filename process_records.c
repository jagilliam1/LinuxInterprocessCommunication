#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <semaphore.h>
#include <pthread.h>
#include "report_record_formats.h"
#include "queue_ids.h"

//Declared outside main for use of status report generator
int* records_read;    //Memory to keep track of progress of each report
int report_count;
int total_records_read;
pthread_mutex_t lock;  //lock for protecting critical sections
pthread_cond_t c;       //condition variable for waking our waiting thread

void sigInt(int sig_number){
    pthread_cond_signal(&c);
    fprintf(stderr, "***Records***\n");
    fprintf(stderr, "%d records read for %d reports\n",total_records_read,report_count);
    for(int i=0; i < report_count; i++){
        fprintf(stderr, "Records sent for report index %d: %d\n",i+1, records_read[i]);
    }
}

void* startStatusReportThread(void* arg){
    pthread_mutex_lock(&lock);
    signal(SIGINT, sigInt); //hook the signal
    pthread_cond_wait(&c,&lock);   //wait on c until SIGINT occurs and wake in SIGINT
    pthread_mutex_unlock(&lock);
}

int main(int argc, char**argv)
{
    int msqid;
    int msgflg = IPC_CREAT | 0666;
    key_t key;
    report_request_buf rbuf;
    report_record_buf sbuf;
    size_t buf_length;
    total_records_read = 0;


    //Create unique key and get our msqid here
    key = ftok(FILE_IN_HOME_DIR,QUEUE_NUMBER);
    if (key == 0xffffffff) {
        fprintf(stderr,"Key cannot be 0xffffffff..fix queue_ids.h to link to existing file\n");
        return 1;
    }
    if ((msqid = msgget(key, msgflg)) < 0) {
        int errnum = errno;
        fprintf(stderr, "Value of errno: %d\n", errno);
        perror("(msgget)");
        fprintf(stderr, "Error msgget: %s\n", strerror( errnum ));
    }
    else
        fprintf(stderr, "msgget: msgget succeeded: msgqid = %d\n", msqid);


    //Receive our first request here to get our report_count
    //msgrcv to receive report request
    int ret;
    do {
      ret = msgrcv(msqid, &rbuf, sizeof(rbuf), 1, 0);//receive type 1 message

      int errnum = errno;
      if (ret < 0 && errno !=EINTR){
        fprintf(stderr, "Value of errno: %d\n", errno);
        perror("Error printed by perror");
        fprintf(stderr, "Error receiving msg: %s\n", strerror( errnum ));
      }
    } while ((ret < 0 ) && (errno == 4));
    //fprintf(stderr,"msgrcv error return code --%d:$d--",ret,errno);

    fprintf(stderr,"process-msgrcv-request: msg type-%ld, Record %d of %d: %s ret/bytes rcv'd=%d\n", rbuf.mtype, rbuf.report_idx,rbuf.report_count,rbuf.search_string, ret);


    //We will populate our rbuf array with all of our request before processing
    //Store in variable for saftety. Value should not change
    report_count = rbuf.report_count;
    
    report_request_buf* rbuf_array; //array to hold our report_request from buffer
    rbuf_array = malloc(sizeof(report_request_buf) * (report_count));


    //Before processing remaining request store our first request (used to get report_count) in our array array
    rbuf_array[rbuf.report_idx-1] = rbuf;   //-1 because have to shift left because report_idx starts a

    //Process rest of request
    for(int i=1; i < report_count; i++){

        int ret;
        do {
            ret = msgrcv(msqid, &rbuf, sizeof(rbuf), 1, 0);//receive type 1 message

            int errnum = errno;
            if (ret < 0 && errno !=EINTR){
                fprintf(stderr, "Value of errno: %d\n", errno);
                perror("Error printed by perror");
                fprintf(stderr, "Error receiving msg: %s\n", strerror( errnum ));
            }
        } while ((ret < 0 ) && (errno == 4));   

        rbuf_array[rbuf.report_idx-1] = rbuf;   //-1 because have to shift left because report_idx starts at 1
    }


    //Create a semaphore array here that will hold our values used in printing status report
    records_read = malloc(sizeof(int) * (report_count+1));
    //init our semaphores with values here
    for(int i=0; i < report_count; i++){
        records_read[i]=0;
    }

    //Then create our thread here and put it so sleep
    pthread_t status_report_thread;
    pthread_create(&status_report_thread, NULL, startStatusReportThread, NULL); //Start status report thread


    //Allocate our record_content and search_string arrays
    char record_content[RECORD_MAX_LENGTH];
    char curr_search_string[SEARCH_STRING_MAX_LENGTH];
    char* test_string;
    int sleep_count=0;
    
    //Now loop through each record
    while(fgets(record_content,RECORD_MAX_LENGTH,stdin) != NULL){

        //updating shared variable
        pthread_mutex_lock(&lock);
        total_records_read++;
        pthread_mutex_unlock(&lock);

        //Once we have read the record, loop to check if any current search strings are in the record
        for(int i=0; i < report_count; i++){
            strcpy(curr_search_string, rbuf_array[i].search_string);    //Set current search string
            test_string = strstr(record_content, curr_search_string);   //Search for current search string and store the result in test_string

            //Search string is in record. Send record to queue
            if(test_string != NULL){
                
                //Get current key and msqid for our current report_index
                key = ftok(FILE_IN_HOME_DIR,i+1);
                if (key == 0xffffffff) {
                    fprintf(stderr,"Key cannot be 0xffffffff..fix queue_ids.h to link to existing file\n");
                    return 1;
                }
                if ((msqid = msgget(key, msgflg)) < 0) {
                    int errnum = errno;
                    fprintf(stderr, "Value of errno: %d\n", errno);
                    perror("(msgget)");
                    fprintf(stderr, "Error msgget: %s\n", strerror( errnum ));
                }
                else{
                    fprintf(stderr, "msgget: msgget succeeded: msgqid = %d\n", msqid);
                }


                // We'll send message type 2
                sbuf.mtype = 2;
                strcpy(sbuf.record, record_content); 
                buf_length = strlen(sbuf.record) + sizeof(int)+1;//struct size without

                //Lock here because we dont want to be writing and trying to print status report at same time
                pthread_mutex_lock(&lock);
                // Send a message.
                if((msgsnd(msqid, &sbuf, buf_length, IPC_NOWAIT)) < 0) {
                    int errnum = errno;
                    fprintf(stderr,"%d, %ld, %s %d\n", msqid, sbuf.mtype, sbuf.record, (int)buf_length);
                    perror("(msgsnd)");
                    fprintf(stderr, "Error sending msg: %s\n", strerror( errnum ));
                    exit(1);
                }
                else{
                    fprintf(stderr,"msgsnd-report_record: record\"%s\" Sent (%d bytes)\n", sbuf.record,(int)buf_length);
                }

                records_read[i]++;
                pthread_mutex_unlock(&lock);
            }

        }

            //After 10 records read in sleep for 5 seconds
            sleep_count++;
            if(sleep_count==10){
                sleep_count=0;
                sleep(5);
            }

    }

   //Send 0 length report to all threads to end
   for(int i=0; i < report_count; i++){
       //Get current key and msqid for our current thread to end
        key = ftok(FILE_IN_HOME_DIR,i+1);
        if (key == 0xffffffff) {
            fprintf(stderr,"Key cannot be 0xffffffff..fix queue_ids.h to link to existing file\n");
            return 1;
        }
        if ((msqid = msgget(key, msgflg)) < 0) {
            int errnum = errno;
            fprintf(stderr, "Value of errno: %d\n", errno);
            perror("(msgget)");
            fprintf(stderr, "Error msgget: %s\n", strerror( errnum ));
        }
        else{
            fprintf(stderr, "msgget: msgget succeeded: msgqid = %d\n", msqid);
        }

        // We'll send message type 2.
        sbuf.mtype = 2;
        sbuf.record[0]=0;
        buf_length = strlen(sbuf.record) + sizeof(int)+1;//struct size without
        // Send a message.
        if((msgsnd(msqid, &sbuf, buf_length, IPC_NOWAIT)) < 0) {
            int errnum = errno;
            fprintf(stderr,"%d, %ld, %s, %d\n", msqid, sbuf.mtype, sbuf.record, (int)buf_length);
            perror("(msgsnd)");
            fprintf(stderr, "Error sending msg: %s\n", strerror( errnum ));
            exit(1);
        }
        else
            fprintf(stderr,"msgsnd-report_record: record\"%s\" Sent (%d bytes)\n", sbuf.record,(int)buf_length);
    
   }

    //Send final status report signal
    sigInt(SIGINT);
    int join;   //Catch the return value  of pthread_join
    pthread_join(status_report_thread,(void*)&join);

    //Free any malloced data
    free(rbuf_array);
    free(records_read);

    exit(0);
}

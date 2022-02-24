#include "edu_cs300_MessageJNI.h"
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <signal.h>
#include <jni.h>
#include "report_record_formats.h"
#include "queue_ids.h"


#ifndef darwin
size_t                  /* O - Length of string */
strlcpy(char       *dst,        /* O - Destination string */
        const char *src,      /* I - Source string */
        size_t      size)     /* I - Size of destination string buffer */
{
    size_t    srclen;         /* Length of source string */


    /*
     * Figure out how much room is needed...
     */

    size --;

    srclen = strlen(src);

    /*
     * Copy the appropriate amount...
     */

    if (srclen > size)
        srclen = size;

    memcpy(dst, src, srclen);
    dst[srclen] = '\0';

    return (srclen);
}

#endif


/*
 * Class:     edu_cs300_MessageJNI
 * Method:    writeReportRequest
 * Signature: (IILjava/lang/String;Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_edu_cs300_MessageJNI_writeReportRequest
  (JNIEnv *env, jclass obj, jint report_idx, jint report_count, jstring search_string_parm){

    int msqid;
    int msgflg = IPC_CREAT | 0666;
    key_t key;
    report_request_buf rbuf;

    key = ftok(FILE_IN_HOME_DIR ,QUEUE_NUMBER);
    if (key == 0xffffffff) {
        fprintf(stderr,"Key cannot be 0xffffffff..fix queue_ids.h to link to existing file\n");
        fprintf(stderr,"Ctl-C and fix the problem\n");
        return;
    }

    if ((msqid = msgget(key, msgflg)) < 0) {
        int errnum = errno;
        fprintf(stderr, "Value of errno: %d\n", errno);
        perror("(msgget)");
        fprintf(stderr, "Error sending msg: %s\n", strerror( errnum ));
    }
    else
        fprintf(stderr, "msgget: msgget succeeded: msgqid = %d\n", msqid);

    const char *search_string = (*env)->GetStringUTFChars(env,search_string_parm, NULL);
    int search_string_len = strlen(search_string);

    int buffer_length=sizeof(report_request_buf)-sizeof(long); //int

    // // We'll send message type 1
    rbuf.mtype = 1;

    rbuf.report_idx=report_idx; //index of response
    rbuf.report_count=report_count; //total excerpts available

    strlcpy(rbuf.search_string,search_string,SEARCH_STRING_FIELD_LENGTH);

    // Send a message.
    if((msgsnd(msqid, &rbuf, buffer_length, IPC_NOWAIT)) < 0) {
        int errnum = errno;

        perror("(msgsnd)");
        fprintf(stderr, " resulted in error sending msg: %s\n", strerror( errnum ));
        exit(1);
    }
    else
        fprintf(stderr," successfully sent\n");

    fprintf(stderr,"JNI msgsnd-report_request: Record %d of %d: search for %s...%p\n",rbuf.report_idx,rbuf.report_count,rbuf.search_string,env);

    (*env)->ReleaseStringUTFChars(env, search_string_parm, search_string);
}

/*
 * Class:     edu_cs300_MessageJNI
 * Method:    readReportRecord
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_edu_cs300_MessageJNI_readReportRecord
  (JNIEnv *env, jclass obj, jint qid){

    key_t key;
    int msqid;
    report_record_buf report_rec_buf;


    //const char *queue_string_cmp = (*env)->GetStringUTFChars(env,queue_string, NULL);

    // ftok to generate unique key
    //key = ftok(CRIMSON_ID, QUEUE_NUMBER);
    key = ftok(FILE_IN_HOME_DIR, qid);
    if (key == 0xffffffff) {
        fprintf(stderr,"Key cannot be 0xffffffff..fix queue_ids.h to link to existing file\n");
        fprintf(stderr,"returned record is not valid\n");
    }
    // msgget creates a message queue
    // and returns identifier
    msqid = msgget(key, 0666 | IPC_CREAT);

    // msgrcv to receive message
    int ret = msgrcv(msqid, &report_rec_buf, sizeof(report_rec_buf), 2, 0);//TODO what is the correct length here
    if (ret < 0) {
        int errnum = errno;
        fprintf(stderr, "Value of errno: %d\n", errno);
        perror("Error printed by perror");
        fprintf(stderr, "Error receiving msg: %s\n", strerror( errnum ));

    }
    fprintf(stderr,"JNI msgrcv-report_rec: Report record:%s ret bytes rcv'd %d...%p\n",report_rec_buf.record,ret,env);


    // Create the object of the class UserData
    jstring result;

    puts(report_rec_buf.record);
    result = (*env)->NewStringUTF(env,report_rec_buf.record);
    return result;
}


JNIEXPORT jstring JNICALL Java_edu_cs300_MessageJNI_readStringMsg
(JNIEnv *env, jobject obj) {

    key_t key;
    int msqid;
    char rbuf[100];
    int msgflg = IPC_CREAT | 0666;

    // ftok to generate unique key
    key = ftok(FILE_IN_HOME_DIR, QUEUE_NUMBER);

    // msgget creates a message queue
    // and returns identifier
    if ((msqid = msgget(key, msgflg)) < 0) {
        int errnum = errno;
        fprintf(stderr, "Value of errno: %d\n", errno);
        perror("(msgget)");
        fprintf(stderr, "Error msgget: %s\n", strerror( errnum ));
    }
    else
        fprintf(stderr, "msgget: msgget succeeded: msgqid = %d\n", msqid);

    // msgrcv to receive message
    int ret = msgrcv(msqid, &rbuf, 100, 1, 0);//TODO what is the correct length here
    if (ret < 0) {
        int errnum = errno;
        fprintf(stderr, "Value of errno: %d\n", errno);
        perror("Error printed by perror");
        fprintf(stderr, "Error receiving msg: %s\n", strerror( errnum ));
        strcpy(rbuf,"error");//return error to java program
    }

    jstring result;

    puts(rbuf);
    result = (*env)->NewStringUTF(env,rbuf);
    return result;
}



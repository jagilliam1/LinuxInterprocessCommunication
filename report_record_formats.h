#ifndef LWS_H
#define LWS_H


#define SEARCH_STRING_MAX_LENGTH 10
#define RECORD_MAX_LENGTH 80
#define SEARCH_STRING_FIELD_LENGTH SEARCH_STRING_MAX_LENGTH+1
#define RECORD_FIELD_LENGTH RECORD_MAX_LENGTH+1

// Declare the message structure

//Report request from ReportingSystem
typedef struct reportrequestbuf {
  long mtype;
  int report_idx;
  int report_count;
  char search_string[SEARCH_STRING_FIELD_LENGTH];
} report_request_buf;


//Report record

typedef struct reportrecordbuf {
	long mtype;
	  char record[RECORD_FIELD_LENGTH];
} report_record_buf;
#endif

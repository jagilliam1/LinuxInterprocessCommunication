# Define a variable for classpath
CLASS_PATH = .
JAVA_HOME=/usr/java/latest
#JAVA_HOME=/usr/lib/jvm/java-11-openjdk-amd64 #will need to remeber to change this once I test on server/turn in
JAVA_PKG=edu/cs300


UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	OSFLAG := linux
	SHARED_LIB := libsystem5msg.so
	LINK_FLAGS := -shared
endif
ifeq ($(UNAME_S),Darwin)
	JAVA_HOME=/Library/Java/JavaVirtualMachines/jdk1.8.0_111.jdk/Contents/Home
	OSFLAG := darwin
	SHARED_LIB := libsystem5msg.dylib
	LINK_FLAGS := -dynamiclib
endif


all:
	@echo $(OSFLAG)

.SUFFIXES: .java .class

.java.class:
	javac -classpath $(CLASS_PATH) $(JAVA_PKG)/*.java

classes: $(CLASSES:.java=.class)

CLASSES = \
	$(JAVA_PKG)/ReportingSystem.java \
        $(JAVA_PKG)/MessageJNI.java \
	$(JAVA_PKG)/DebugLog.java 

classes: $(CLASSES:.java=.class)

all : edu_cs300_MessageJNI.h process_records $(JAVA_PKG)/ReportingSystem.class $(SHARED_LIB) classes msgsnd msgrcv

edu_cs300_MessageJNI.h: $(JAVA_PKG)/MessageJNI.java
	javac -h . $(JAVA_PKG)/MessageJNI.java
    
process_records:process_records.c report_record_formats.h
	gcc -std=c99 -pthread -D_GNU_SOURCE $(MAC_FLAG) process_records.c -o process_records

edu_cs300_MessageJNI.o:report_record_formats.h edu_cs300_MessageJNI.h system5_msg.c queue_ids.h
	gcc -c -fPIC -I${JAVA_HOME}/include -I${JAVA_HOME}/include/$(OSFLAG) -D$(OSFLAG) system5_msg.c -o edu_cs300_MessageJNI.o

$(SHARED_LIB):report_record_formats.h edu_cs300_MessageJNI.h edu_cs300_MessageJNI.o
	gcc $(LINK_FLAGS) -o $(SHARED_LIB) edu_cs300_MessageJNI.o -lc

test: process_records $(JAVA_PKG)/MessageJNI.class $(SHARED_LIB)
	./msgsnd
	java -cp . -Djava.library.path=. edu/cs300/MessageJNI
	./msgrcv


msgsnd: msgsnd_report_record.c report_record_formats.h queue_ids.h
	gcc -std=c99 -D_GNU_SOURCE -D$(OSFLAG) msgsnd_report_record.c -o msgsnd

msgrcv: msgrcv_report_request.c report_record_formats.h queue_ids.h
	gcc -std=c99 -D_GNU_SOURCE msgrcv_report_request.c -o msgrcv


clean :
	rm *.o $(SHARED_LIB) edu_cs300_MessageJNI.h $(JAVA_PKG)/*.class process_records msgsnd msgrcv
	ipcs -q | grep ${USER} | awk ' { print $$2 } ' | xargs -I {} ipcrm -q {}
#ipcs -q|grep ${USER}|while read line; do id=`echo $$line|cut -d' ' -f3`; echo $$id; ipcrm -Q $$id;done


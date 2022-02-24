This project is a demonstration of a system that runs a Java and a C program that concurrently passes data between each program. This was done using Linux message queues.
Part of this project was already created before hand. I implemented the process_records.c and ReportingSystem.java
The process_records.c just does some processing then sends the processed message to ReportingSystem.java.
ReportingSystem.java takes that message and prints a report based off a formating file that was provided

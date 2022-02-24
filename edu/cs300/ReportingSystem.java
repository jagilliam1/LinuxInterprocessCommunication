package edu.cs300;

import java.io.File;
import java.io.FileNotFoundException;
import java.util.Enumeration;
import java.util.Scanner;
import java.util.Vector;
import java.io.FileWriter;
import java.text.BreakIterator;

public class ReportingSystem {


	public ReportingSystem() {
	  DebugLog.log("Starting Reporting System");
	}

	//This is the class that we will use for creating our multiple threads for each report file
	static class reportThread extends Thread{
	private int threadID;
	public String reportFile;
	private int reportCount;	//include this here to send with our System V message
	
	public reportThread(int threadID, String reportFile, int reportCount){
		this.threadID = threadID;
		this.reportFile = reportFile;
		this.reportCount = reportCount;
	}

	//Here is where we will do all actions for each report thread
	@Override
	public void run(){

		String reportTitle;
		String searchString;
		String outputFileName;
		String reportRecord;
		Vector<String> columnNames = new Vector<String>();
		Vector<Integer> dataFieldColumns = new Vector<Integer>();

		File report = new File(this.reportFile);
		if (!report.exists()) return;	//If the file does not exist exit

		//Scanner to actually store our values
		Scanner reportFileScanner;
		try {
			reportFileScanner = new Scanner(report);

			reportTitle = reportFileScanner.nextLine();
			searchString = reportFileScanner.nextLine();
			outputFileName = reportFileScanner.nextLine();

			String line;
			//Read in our dataFieldColumns and our columnNames from report specification file
			while(!(line = reportFileScanner.nextLine()).isEmpty()){
				String splitLine[];
				splitLine = line.split(",|-|\\n|\\z");
				dataFieldColumns.addElement(Integer.valueOf(splitLine[0]));
				dataFieldColumns.addElement(Integer.valueOf(splitLine[1]));
				columnNames.addElement(splitLine[2]);
			}
			reportFileScanner.close();


			//Create our output file
			File outputFile = new File(outputFileName);
			try{
				if(!outputFile.exists())	//check if the outputFile already exists
				outputFile.createNewFile();	//If we cant create file stop processing
			}	catch(Exception e){
				System.err.println("Could not create Output File. Threw Exception" +e);
			}

			//Write report
			try{
				//Format to our initial output file
				FileWriter outputFileWriter = new FileWriter(outputFileName);
				outputFileWriter.write(reportTitle + '\n');
				for(int i=0; i < columnNames.size(); i++){
					outputFileWriter.append(columnNames.elementAt(i) + '\t');
				}
				outputFileWriter.write('\n');	//add our newline at end of columns

				//Now once we do this send record request to process_records. Then parse the string we get back with our column heads in this report to write report
				MessageJNI.writeReportRequest(this.threadID, this.reportCount, searchString);

				//Keep reading in record request until it sends an empty record then we break out of receving records.
				while(true){
					reportRecord = MessageJNI.readReportRecord(this.threadID);
					if(reportRecord.length()==0) break;		//If we receive a 0 length record we are done writing records

					String dataField = "";	//String will be used in our loops to hold each specific data field

					
					//Outer loop handles getting our data field postions out of dataFieldColumns
					for(int j=0; j < dataFieldColumns.size(); j = j + 2){
						//Inner loop gets our data column postions. Inital position = k . Final position (Inclusive) = dataFieldColumns.elementAt(j+1)
						for(int k = dataFieldColumns.elementAt(j)-1; k < dataFieldColumns.elementAt(j+1) ; k++){
							dataField = dataField + reportRecord.charAt(k);
						}
						outputFileWriter.write(dataField + '\t');
						dataField="";
					}
					outputFileWriter.write('\n');
				}
				outputFileWriter.close();

			}	catch(Exception e){
				System.err.println("Something went wrong with Writing Report. Threw Exception" +e);
			}

		} catch (FileNotFoundException e1) {
			System.err.println("Something went wrong with reading from Report Specification. Threw FileNotFoundException:" + e1);
		}

	}

}

	public static void main(String[] args) throws FileNotFoundException {

		   //ReportingSystem reportSystem= new ReportingSystem();

		   //Open report_list file and begin scanning in our values into numReports and report_list_array
		   File report_list = new File("report_list.txt");
		   if (!report_list.exists()) return;	//If the file does not exist exit
		   Scanner report_list_scanner = new Scanner(report_list);

		   int numReports = report_list_scanner.nextInt();
		   report_list_scanner.nextLine();	//Move scanner to next line. The nextInt function doesn't receive the entire line just the int
		   String report_list_array[] = new String[numReports];

		   for(int i=0; i < numReports; i++){
			   report_list_array[i] = report_list_scanner.nextLine();
		   }
		   report_list_scanner.close();
		  
		   reportThread reportThreads[] = new reportThread[numReports];

		   //Create an array of threads based on our numReports and start their execution
		   for(int i = 0; i < numReports; i++){
				reportThreads[i] = new reportThread(i+1, report_list_array[i], numReports);
				reportThreads[i].start();
		   }

		   //Once we have started the threads and finish join them
		    for(int i = 0; i < numReports; i++){
				try{
					reportThreads[i].join();
				}	catch(Exception e){
					System.err.println("Something went wrong with joining threads. Threw Exception" +e);
				}
	   		}
		   
	}

}

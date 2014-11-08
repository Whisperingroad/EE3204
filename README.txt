
 
Develop a TCP-based client-server socket program for transferring a large message. Here, the message transmitted from the client to server is read from a large file. The message is split into short data-units which are sent by using stop-and-wait flow control. Also, a data-unit sent could be damaged with some error probability. Verify if the file has been sent completely and correctly by comparing the received file with the original file. Measure the message transfer time and throughput for various sizes of data-units. Also, measure the performance for various error probabilities and also for the error-free scenario. 



the example is to show how to transmit a large file using small packets. the file to be sent is "myfile.txt", the received data is stored in "myTCPreceive.txt" in TCP case and in "myUDPreceive.txt" in UDP case.
the packet size is fixed at 100 bytes per packets. the receiver transmit the acknolegement to sender when the last byte is received. In test, the file size is 50554 bytes. In TCP case, all data is received without error. 

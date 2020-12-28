package net;

import java.io.*;
import java.net.ServerSocket;
import java.net.Socket;

public class HTTPServerSkeleton {
    static final int PORT = 6789;
    static final int BUFFER_SIZE = 4096;

    public static void main(String[] args) throws IOException {
        File logFile = new File("logfile.txt");
        BufferedReader br = new BufferedReader(new FileReader(logFile));

        ServerSocket serverConnect = new ServerSocket(PORT);
        System.out.println("Server started.\nListening for connections on port : " + PORT + " ...\n");

        while(true)
        {
            Socket socket = serverConnect.accept();
            ServerThread serverThread = new ServerThread(socket, BUFFER_SIZE);

            //socket.close();
        }
    }
    
}

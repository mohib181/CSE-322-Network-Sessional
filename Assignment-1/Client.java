package net;

import java.io.IOException;
import java.net.Socket;
import java.net.UnknownHostException;
import java.util.Scanner;

public class Client {
    static final int BUFFER_SIZE = 4096;

    public static void main(String[] args) {
        String serverAddress = "localhost";
        int serverPort = 6789;

        String clientInput;
        Scanner scanner = new Scanner(System.in);

        while (true) {
            try {
                System.out.println("Please Enter in this format");
                System.out.println("UPLOAD file_name");

                clientInput = scanner.nextLine();
                if (clientInput.startsWith("UPLOAD")) {
                    String[] data = clientInput.split(" ", 2);

                    Socket socket = new Socket(serverAddress, serverPort);
                    ClientThread clientThread = new ClientThread(socket, data[1], BUFFER_SIZE);
                }
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }
}


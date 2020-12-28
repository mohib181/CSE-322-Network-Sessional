package net;

import java.io.*;
import java.net.Socket;

public class ClientThread implements Runnable {
    int bufferSize;
    Thread thread;
    Socket socket;
    String fileName;

    public ClientThread(Socket socket, String fileName, int bufferSize) {
        this.socket = socket;
        this.fileName = fileName;
        this.bufferSize = bufferSize;
        this.thread = new Thread(this);
        this.thread.start();
    }

    @Override
    public void run() {
        try {
            PrintWriter pr = new PrintWriter(socket.getOutputStream());

            String filePath = fileName;
            File file = new File(filePath);

            if(file.exists()) {
                pr.write("UPLOAD " + fileName + "\r\n");
                pr.flush();

                int count;
                byte[] buffer = new byte[bufferSize];
                OutputStream out = socket.getOutputStream();
                BufferedInputStream bis = new BufferedInputStream(new FileInputStream(file));
                while ((count = bis.read(buffer)) >= 0) {
                    out.write(buffer, 0, count);
                    out.flush();
                }
                bis.close();
                out.close();

                System.out.println("\nUPLOAD SUCCESSFUL\n");
            }
            else {
                System.out.println("UPLOAD FAILED");
                pr.write("UPLOAD FAILED" + "\r\n");
                pr.flush();
            }

            //socket.close();
            //System.out.println("Socket Closed");

        } catch (IOException e) {
            e.printStackTrace();
        }
    }
}

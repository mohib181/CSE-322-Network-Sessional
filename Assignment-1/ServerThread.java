package net;

import java.io.*;
import java.net.Socket;
import java.util.Date;

public class ServerThread implements Runnable {
    Thread thread;
    Socket socket;
    int bufferSize;
    String htmlHead = "<html>\n" +
            "\t<head>\n" +
            "\t\t<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\">\n" +
            "\t</head>\n" +
            "\t<body>\n";
    String htmlEnd = "\n\t</body>\n" +
            "</html>\n\n";

    public ServerThread(Socket socket, int bufferSize) {
        this.socket = socket;
        this.bufferSize = bufferSize;
        this.thread = new Thread(this);
        thread.start();
    }

    private File getFile(String pathAddress) {
        //System.out.println(pathAddress);
        String[] pathEntry = pathAddress.split("/");
        StringBuilder sb = new StringBuilder("root");

        for (int i = 1; i < pathEntry.length; i++) {
            if (pathEntry[i].equals("root")) continue;
            sb.append("//");
            sb.append(pathEntry[i].replace("%20", " "));
        }

        String filePath = sb.toString();
        //System.out.println(filePath);

        return new File(filePath);
    }

    private String getContent(File file) {
        StringBuilder sb = new StringBuilder();
        File[] fileNames = file.listFiles();

        for(File f: fileNames) {
            if (f.isDirectory()) {
                sb.append("\t\t<a href=\"/" + f.getPath() + "\"><b>" + f.getName() + "</b></a><br>\n");
            }
            else {
                sb.append("\t\t<a href=\"/" + f.getPath() + "\">" + f.getName() + "</a><br>\n");
            }
        }
        String body = sb.toString();
        return htmlHead + body + htmlEnd;
    }

    @Override
    public void run() {
        try {
            PrintWriter logWriter = new PrintWriter(new FileWriter("logfile.txt", true));
            PrintWriter pr = new PrintWriter(socket.getOutputStream());
            OutputStream out = socket.getOutputStream();
            BufferedReader in = new BufferedReader(new InputStreamReader(socket.getInputStream()));

            String input = in.readLine();
            //System.out.println("FOUND INPUT: " + input);

            //String content = "<html>Hello</html>";
            //if(input == null) continue;
            if(input != null && input.length() > 0) {
                logWriter.write("Client Request Message:\r\n");
                logWriter.write(input);
                logWriter.write("\r\n");

                logWriter.write("\r\n");
                logWriter.write("Server Response Message:\r\n");

                if(input.startsWith("GET")) {

                    String[] data = input.split(" ");
                    File file = getFile(data[1]);

                    if (file.exists()) {
                        if (file.isDirectory()) {
                            String content = getContent(file);
                            pr.write("HTTP/1.1 200 OK\r\n");
                            pr.write("Server: Java HTTP Server: 1.0\r\n");
                            pr.write("Date: " + new Date() + "\r\n");
                            pr.write("Content-Type: text/html\r\n");
                            pr.write("Content-Length: " + content.length() + "\r\n");
                            pr.write("\r\n");
                            pr.write(content);
                            pr.flush();


                            //writing in the log file
                            logWriter.write("HTTP/1.1 200 OK\r\n");
                            logWriter.write("Server: Java HTTP Server: 1.0\r\n");
                            logWriter.write("Date: " + new Date() + "\r\n");
                            logWriter.write("Content-Type: text/html\r\n");
                            logWriter.write("Content-Length: " + content.length() + "\r\n");
                            logWriter.write("\r\n");
                            logWriter.write(content);
                            logWriter.write("\r\n");
                            logWriter.flush();
                        }
                        else {
                            //System.out.println("file length: " + file.length());
                            pr.write("HTTP/1.1 200 OK\r\n");
                            pr.write("Server: Java HTTP Server: 1.0\r\n");
                            pr.write("Date: " + new Date() + "\r\n");
                            pr.write("Content-Type: application/x-force-download" + "\r\n");
                            pr.write("Content-Length: " + file.length() + "\r\n");
                            pr.write("\r\n");
                            pr.flush();

                            //pr.write(content);
                            int count;
                            byte[] buffer = new byte[bufferSize];

                            BufferedInputStream bis = new BufferedInputStream(new FileInputStream(file));
                            while ((count = bis.read(buffer)) >= 0) {
                                out.write(buffer, 0, count);
                                out.flush();
                            }
                            bis.close();

                            //writing in the log file
                            logWriter.write("HTTP/1.1 200 OK\r\n");
                            logWriter.write("Server: Java HTTP Server: 1.0\r\n");
                            logWriter.write("Date: " + new Date() + "\r\n");
                            logWriter.write("Content-Type: application/x-force-download" + "\r\n");
                            logWriter.write("Content-Length: " + file.length() + "\r\n");
                            logWriter.write("\r\n");
                            logWriter.flush();

                        }
                    }
                    else {
                        pr.write("HTTP/1.1 404 Not found\r\n");
                        pr.write("Server: Java HTTP Server: 1.0\r\n");
                        pr.write("Date: " + new Date() + "\r\n");
                        pr.write("\r\n");
                        pr.flush();

                        System.out.println("HTTP/1.1 404 Not found\n");

                        logWriter.write("HTTP/1.1 404 Not found\r\n");
                        logWriter.write("Server: Java HTTP Server: 1.0\r\n");
                        logWriter.write("Date: " + new Date() + "\r\n");
                        logWriter.write("\r\n");
                        logWriter.flush();
                    }
                }

                else if (input.startsWith("UPLOAD")) {
                    //System.out.println(input);
                    String[] data = input.split(" ", 2);
                    String pathAddress = "root//" + data[1];

                    //System.out.println(pathAddress);
                    if (pathAddress.equals("root//FAILED")) {
                        System.out.println("UPLOAD FAILED\n");
                        logWriter.write("UPLOAD FAILED" + "\r\n");
                        logWriter.flush();
                    }
                    else {
                        int count;
                        byte[] buffer = new byte[bufferSize];

                        FileOutputStream fos = new FileOutputStream(pathAddress);
                        InputStream ins = socket.getInputStream();

                        System.out.println("path: " + pathAddress);
                        while((count=ins.read(buffer)) >= 0){
                            fos.write(buffer, 0, count);
                        }
                        fos.close();

                        System.out.println("UPLOAD SUCCESSFUL");
                        logWriter.write("UPLOAD SUCCESSFUL" + "\r\n");
                        logWriter.flush();
                    }
                }
            }

            socket.close();
            //System.out.println("Socket Closed");

        } catch (IOException e) {
            e.printStackTrace();
        }
    }
}

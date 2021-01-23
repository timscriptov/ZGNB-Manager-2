package com.zengge.nbmanager.utils;

import android.content.Context;
import android.widget.Toast;

import com.zengge.nbmanager.Features;

import org.jetbrains.annotations.NotNull;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;

public class FormatingCode {
    public static boolean formattingCode(Context ctx, String fn_path) {
        String textIn = readCodeFromFile(ctx,fn_path);
        String options = "-style=java";
        String textOut = Features.AStyleMain(textIn, options);
        if(textIn.isEmpty()) return false;
        if(!writeCodeToFile(ctx,textOut, fn_path)) return false;
        return true;
    }

    public static @NotNull String readCodeFromFile(Context ctx, String filePath) {

        File inFile = new File(filePath);
        final int readSize =  131072;
        StringBuffer bufferIn = new StringBuffer(readSize);
        char fileIn[] = new char[readSize];

        try {
            BufferedReader in =
                    new BufferedReader(new FileReader(inFile));

            int charsIn = in.read(fileIn, 0, readSize);
            while(charsIn != -1) {
                bufferIn.append(fileIn, 0, charsIn);
                charsIn = in.read(fileIn, 0, readSize);
            }
            in.close();
        } catch(Exception e) {
            if(e instanceof FileNotFoundException) {
                Toast.makeText(ctx, "Cannot open input file " + filePath, Toast.LENGTH_LONG).show();
                return "";
            } else if(e instanceof IOException) {
                Toast.makeText(ctx, "Error reading file " + filePath, Toast.LENGTH_LONG).show();
                return "";
            } else {
                Toast.makeText(ctx, e.getMessage() + " " + filePath, Toast.LENGTH_LONG).show();
                return "";
            }
        }
        return bufferIn.toString();
    }

    public static boolean writeCodeToFile(Context ctx,String textOut, String filePath) {

        String origfilePath = filePath +  ".orig";
        File origFile = new File(origfilePath);
        File outFile = new File(filePath);
        origFile.delete();
        if(!outFile.renameTo(origFile)) {
            Toast.makeText(ctx, "Cannot create backup file " + origfilePath, Toast.LENGTH_LONG).show();
            return false;
        }

        try {
            BufferedWriter out =
                    new BufferedWriter(new FileWriter(filePath));
            out.write(textOut, 0, textOut.length());
            out.close();
        } catch(IOException e) {
            Toast.makeText(ctx, "Cannot write to output " + filePath, Toast.LENGTH_LONG).show();
            return false;
        }
        return true;
    }
}

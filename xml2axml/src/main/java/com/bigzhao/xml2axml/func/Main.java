package com.bigzhao.xml2axml.func;

import android.content.Context;
import android.util.Log;

import com.bigzhao.xml2axml.Encoder;
import com.bigzhao.xml2axml.android.content.res.AXmlResourceParser;
import com.bigzhao.xml2axml.utils.FileUtils;

import org.xmlpull.v1.XmlPullParserException;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.PrintStream;

/**
 * Created by Roy on 15-10-6.
 */
public class Main {
    public static void encode(Context context, String input, String output) throws IOException, XmlPullParserException {
        FileUtils.writeBytesToFile(output, new Encoder().encodeFile(context, input));
    }

    public static void decode(String in, String out) throws FileNotFoundException {
        AXMLPrinter.out = new PrintStream(new File(out));
        AXMLPrinter.main(new String[]{in});
        AXMLPrinter.out.close();
    }

    public static boolean isBinAXML(String input) {
        try {
            AXmlResourceParser aXmlResourceParser = new AXmlResourceParser();
            aXmlResourceParser.open(new FileInputStream(input));
            int next = aXmlResourceParser.next();
            Log.d("AXMLJudger", "Succeed parsing AXML and type value is" + next);
            return true;
        } catch (Exception e) {
            e.printStackTrace();
            return false;
        }
    }
}

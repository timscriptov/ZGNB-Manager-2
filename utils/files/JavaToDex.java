package com.zengge.jadx.utils.files;

import com.android.dx.command.dexer.DxContext;
import com.android.dx.command.dexer.Main;
import jadx.core.utils.exceptions.JadxException;
import jadx.core.utils.files.FileUtils;

import java.io.ByteArrayOutputStream;
import java.nio.file.DirectoryStream;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;

public class JavaToDex {
    private String dxErrors;

    private static class DxArgs extends Main.Arguments {
        public DxArgs(DxContext dxContext, String str, String[] strArr) {
            super(dxContext);
            this.outName = str;
            this.fileNames = strArr;
            this.jarOutput = false;
            this.multiDex = true;
            this.optimize = true;
            this.localInfo = true;
            this.coreLibrary = true;
            this.debug = true;
            this.warnings = true;
            this.minSdkVersion = 28;
        }
    }

    public List<Path> convert(Path path) throws JadxException {
        try {
            ByteArrayOutputStream byteArrayOutputStream = new ByteArrayOutputStream();
            ByteArrayOutputStream byteArrayOutputStream2 = new ByteArrayOutputStream();
            DxContext dxContext = new DxContext(byteArrayOutputStream, byteArrayOutputStream2);
            Path createTempDir = FileUtils.createTempDir("jar-to-dex-");
            int runDx = new Main(dxContext).runDx(new DxArgs(dxContext, createTempDir.toAbsolutePath().toString(), new String[]{path.toAbsolutePath().toString()}));
            this.dxErrors = byteArrayOutputStream2.toString("UTF-8");
            if (runDx == 0) {
                ArrayList arrayList = new ArrayList();
                DirectoryStream<Path> newDirectoryStream = Files.newDirectoryStream(createTempDir);
                for (Path next : newDirectoryStream) {
                    arrayList.add(next);
                    next.toFile().deleteOnExit();
                }
                if (newDirectoryStream != null) {
                    newDirectoryStream.close();
                }
                byteArrayOutputStream2.close();
                byteArrayOutputStream.close();
                return arrayList;
            }
            throw new JadxException("Java to dex conversion error, code: " + runDx);
        } catch (Exception e) {
            throw new JadxException("dx exception: " + e.getMessage(), e);
        }
    }

    public String getDxErrors() {
        return this.dxErrors;
    }

    public boolean isError() {
        String str = this.dxErrors;
        return str != null && !str.isEmpty();
    }
}
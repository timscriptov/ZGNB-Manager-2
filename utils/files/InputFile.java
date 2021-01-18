package com.zengge.jadx.utils.files;

import com.android.dex.Dex;
import com.android.dex.DexException;
import com.zengge.jadx.utils.AsmUtils;
import com.zengge.jadx.utils.SmaliUtils;

import jadx.api.plugins.utils.ZipSecurity;
import jadx.core.utils.exceptions.DecodeException;
import jadx.core.utils.exceptions.JadxException;
import jadx.core.utils.exceptions.JadxRuntimeException;
import jadx.core.utils.files.FileUtils;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.nio.file.CopyOption;
import java.nio.file.Files;
import java.nio.file.OpenOption;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.List;
import java.util.jar.JarOutputStream;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class InputFile {
    private static final Logger LOG = LoggerFactory.getLogger((Class<?>) InputFile.class);
    private final List<DexFile> dexFiles = new ArrayList();
    private final File file;

    public static void addFilesFrom(File file2, @NotNull List<InputFile> list, boolean z) throws IOException, DecodeException {
        InputFile inputFile = new InputFile(file2);
        inputFile.searchDexFiles(z);
        list.add(inputFile);
    }

    private InputFile(@NotNull File file2) throws IOException {
        if (file2.exists()) {
            this.file = file2;
            return;
        }
        throw new IOException("File not found: " + file2.getAbsolutePath());
    }

    private void searchDexFiles(boolean z) throws IOException, DecodeException {
        String name = this.file.getName();
        if (name.endsWith(".dex")) {
            addDexFile(name, this.file.toPath());
        } else if (name.endsWith(".smali")) {
            Path createTempFile = FileUtils.createTempFile(".dex");
            SmaliUtils.assembleDex(createTempFile.toAbsolutePath().toString(), this.file.getAbsolutePath());
            addDexFile(name, createTempFile);
        } else if (name.endsWith(".class")) {
            for (Path addDexFile : loadFromClassFile(this.file)) {
                addDexFile(name, addDexFile);
            }
        } else if (FileUtils.isApkFile(this.file) || FileUtils.isZipDexFile(this.file)) {
            loadFromZip(".dex");
        } else if (name.endsWith(".jar") || name.endsWith(".aar")) {
            if (!loadFromZip(".dex")) {
                if (name.endsWith(".jar")) {
                    for (Path addDexFile2 : loadFromJar(this.file.toPath())) {
                        addDexFile(name, addDexFile2);
                    }
                } else if (name.endsWith(".aar")) {
                    loadFromZip(".jar");
                }
            }
        } else if (!z) {
            LOG.warn("No dex files found in {}", (Object) this.file);
        }
    }

    /* JADX WARNING: Can't wrap try/catch for region: R(4:53|(2:55|56)|57|58) */
    /* JADX WARNING: Code restructure failed: missing block: B:15:0x0048, code lost:
        if (r9.endsWith(r15) == false) goto L_0x004a;
     */
    /* JADX WARNING: Code restructure failed: missing block: B:53:0x0102, code lost:
        r15 = move-exception;
     */
    /* JADX WARNING: Code restructure failed: missing block: B:54:0x0103, code lost:
        if (r7 != null) goto L_0x0105;
     */
    /* JADX WARNING: Code restructure failed: missing block: B:56:?, code lost:
        r7.close();
     */
    /* JADX WARNING: Code restructure failed: missing block: B:58:?, code lost:
        throw r15;
     */
    /* JADX WARNING: Missing exception handler attribute for start block: B:57:0x0108 */
    /* JADX WARNING: Removed duplicated region for block: B:48:0x00fb A[SYNTHETIC, Splitter:B:48:0x00fb] */
    /* JADX WARNING: Removed duplicated region for block: B:74:0x0020 A[SYNTHETIC] */
    private boolean loadFromZip(String str) throws IOException, DecodeException {
        ZipFile zipFile = new ZipFile(this.file);
        String str2 = "classes" + str;
        Enumeration<? extends ZipEntry> entries = zipFile.entries();
        int i = 0;
        while (entries.hasMoreElements()) {
            ZipEntry zipEntry = (ZipEntry) entries.nextElement();
            if (ZipSecurity.isValidZipEntry(zipEntry)) {
                String name = zipEntry.getName();
                InputStream inputStream = zipFile.getInputStream(zipEntry);
                if (name.startsWith("classes")) {
                }
                if (!name.endsWith(str2)) {
                    if (name.equals("instant-run.zip") && str.equals(".dex")) {
                        Path createTempFile = FileUtils.createTempFile("instant-run.zip");
                        Files.copy(inputStream, createTempFile, new CopyOption[]{StandardCopyOption.REPLACE_EXISTING});
                        InputFile inputFile = new InputFile(createTempFile.toFile());
                        inputFile.loadFromZip(str);
                        List<DexFile> dexFiles2 = inputFile.getDexFiles();
                        if (!dexFiles2.isEmpty()) {
                            i += dexFiles2.size();
                            this.dexFiles.addAll(dexFiles2);
                        }
                    }
                    if (inputStream == null) {
                        inputStream.close();
                    } else {
                        continue;
                    }
                }
                char c = 65535;
                int hashCode = str.hashCode();
                if (hashCode != 1469737) {
                    if (hashCode == 1475373) {
                        if (str.equals(".jar")) {
                            c = 1;
                        }
                    }
                } else if (str.equals(".dex")) {
                    c = 0;
                }
                if (c != 0) {
                    if (c == 1) {
                        i++;
                        Path createTempFile2 = FileUtils.createTempFile(name);
                        Files.copy(inputStream, createTempFile2, new CopyOption[]{StandardCopyOption.REPLACE_EXISTING});
                        for (Path addDexFile : loadFromJar(createTempFile2)) {
                            addDexFile(name, addDexFile);
                        }
                    } else {
                        throw new JadxRuntimeException("Unexpected extension in zip: " + str);
                    }
                } else if (addDexFile(name, copyToTmpDex(name, inputStream))) {
                    i++;
                }
                if (inputStream == null) {
                }
            }
        }
        zipFile.close();
        if (i > 0) {
            return true;
        }
        return false;
    }

    private boolean addDexFile(String str, Path path) {
        Dex loadDexBufFromPath;
        if (path == null || (loadDexBufFromPath = loadDexBufFromPath(path, str)) == null) {
            return false;
        }
        dexFiles.add(new DexFile(this, str, loadDexBufFromPath, path));
        return true;
    }

    private @Nullable Dex loadDexBufFromPath(Path path, String str) {
        try {
            return new Dex(Files.readAllBytes(path));
        } catch (DexException e) {
            LOG.error("Failed to load dex file: {}, error: {}", str, e.getMessage());
            return null;
        } catch (Exception e2) {
            LOG.error("Failed to load dex file: {}, error: {}", str, e2.getMessage(), e2);
            return null;
        }
    }

    private Path copyToTmpDex(String str, InputStream inputStream) {
        try {
            Path createTempFile = FileUtils.createTempFile(".dex");
            Files.copy(inputStream, createTempFile, new CopyOption[]{StandardCopyOption.REPLACE_EXISTING});
            return createTempFile;
        } catch (Exception e) {
            LOG.error("Failed to load file: {}, error: {}", str, e.getMessage(), e);
            return null;
        }
    }

    private static List<Path> loadFromJar(Path path) throws DecodeException {
        JavaToDex javaToDex = new JavaToDex();
        try {
            LOG.info("converting to dex: {} ...", path.getFileName());
            List<Path> convert = javaToDex.convert(path);
            if (!convert.isEmpty()) {
                if (LOG.isDebugEnabled()) {
                    LOG.debug("result dex files: {}", (Object) convert);
                }
                if (javaToDex.isError()) {
                    LOG.warn("dx message: {}", (Object) javaToDex.getDxErrors());
                }
                return convert;
            }
            throw new JadxException("Empty dx output");
        } catch (Exception e) {
            throw new DecodeException("java class to dex conversion error:\n " + e.getMessage(), e);
        } catch (Throwable th) {
            if (javaToDex.isError()) {
                LOG.warn("dx message: {}", (Object) javaToDex.getDxErrors());
            }
            throw th;
        }
    }

    /* JADX WARNING: Code restructure failed: missing block: B:15:0x0053, code lost:
        r4 = move-exception;
     */
    /* JADX WARNING: Code restructure failed: missing block: B:17:?, code lost:
        r1.close();
     */
    /* JADX WARNING: Code restructure failed: missing block: B:18:0x0057, code lost:
        throw r4;
     */
    private static List<Path> loadFromClassFile(File file2) throws IOException, DecodeException {
        Path createTempFile = FileUtils.createTempFile(".jar");
        JarOutputStream jarOutputStream = new JarOutputStream(Files.newOutputStream(createTempFile, new OpenOption[0]));
        String nameFromClassFile = AsmUtils.getNameFromClassFile(file2);
        if (nameFromClassFile == null || !ZipSecurity.isValidZipEntryName(nameFromClassFile)) {
            throw new IOException("Can't read class name from file: " + file2);
        }
        FileUtils.addFileToJar(jarOutputStream, file2, nameFromClassFile + ".class");
        jarOutputStream.close();
        return loadFromJar(createTempFile);
    }

    public File getFile() {
        return this.file;
    }

    public List<DexFile> getDexFiles() {
        return this.dexFiles;
    }

    public String toString() {
        return this.file.getAbsolutePath();
    }
}
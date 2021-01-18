package com.zengge.nbmanager.code.utils;

public class LiteralTools {
    public static byte parseByte(String str) throws NumberFormatException {
        char[] cArr;
        boolean z;
        if (str == null) {
            throw new NumberFormatException("string is null");
        } else if (str.length() != 0) {
            byte b = 0;
            int i = 1;
            if (str.toUpperCase().endsWith("T")) {
                cArr = str.substring(0, str.length() - 1).toCharArray();
            } else {
                cArr = str.toCharArray();
            }
            byte b2 = 10;
            if (cArr[0] == '-') {
                z = true;
            } else {
                i = 0;
                z = false;
            }
            if (cArr[i] == '0') {
                i++;
                if (i == cArr.length) {
                    return 0;
                }
                if (cArr[i] == 'x' || cArr[i] == 'X') {
                    b2 = 16;
                    i++;
                } else if (Character.digit(cArr[i], 8) >= 0) {
                    b2 = 8;
                }
            }
            byte b3 = (byte) (127 / (b2 / 2));
            while (i < cArr.length) {
                int digit = Character.digit(cArr[i], b2);
                if (digit >= 0) {
                    byte b4 = (byte) (b * b2);
                    if (b > b3) {
                        throw new NumberFormatException(str + " cannot fit into a byte");
                    } else if (b4 >= 0 || b4 < (-digit)) {
                        b = (byte) (b4 + digit);
                        i++;
                    } else {
                        throw new NumberFormatException(str + " cannot fit into a byte");
                    }
                } else {
                    throw new NumberFormatException("The string contains invalid an digit - '" + cArr[i] + "'");
                }
            }
            if (!z || b == Byte.MIN_VALUE) {
                return b;
            }
            if (b >= 0) {
                return (byte) (b * -1);
            }
            throw new NumberFormatException(str + " cannot fit into a byte");
        } else {
            throw new NumberFormatException("string is blank");
        }
    }

    public static int parseInt(String str) throws NumberFormatException {
        boolean z;
        if (str == null) {
            throw new NumberFormatException("string is null");
        } else if (str.length() != 0) {
            char[] charArray = str.toCharArray();
            int i = 10;
            int i2 = 0;
            int i3 = 1;
            if (charArray[0] == '-') {
                z = true;
            } else {
                z = false;
                i3 = 0;
            }
            if (charArray[i3] == '0') {
                i3++;
                if (i3 == charArray.length) {
                    return 0;
                }
                if (charArray[i3] == 'x' || charArray[i3] == 'X') {
                    i = 16;
                    i3++;
                } else if (Character.digit(charArray[i3], 8) >= 0) {
                    i = 8;
                }
            }
            int i4 = Integer.MAX_VALUE / (i / 2);
            while (i3 < charArray.length) {
                int digit = Character.digit(charArray[i3], i);
                if (digit >= 0) {
                    int i5 = i2 * i;
                    if (i2 > i4) {
                        throw new NumberFormatException(str + " cannot fit into an int");
                    } else if (i5 >= 0 || i5 < (-digit)) {
                        i2 = i5 + digit;
                        i3++;
                    } else {
                        throw new NumberFormatException(str + " cannot fit into an int");
                    }
                } else {
                    throw new NumberFormatException("The string contains an invalid digit - '" + charArray[i3] + "'");
                }
            }
            if (!z || i2 == Integer.MIN_VALUE) {
                return i2;
            }
            if (i2 >= 0) {
                return i2 * -1;
            }
            throw new NumberFormatException(str + " cannot fit into an int");
        } else {
            throw new NumberFormatException("string is blank");
        }
    }

    public static long parseLong(String str) throws NumberFormatException {
        char[] cArr;
        if (str == null) {
            throw new NumberFormatException("string is null");
        } else if (str.length() != 0) {
            int i = 0;
            boolean z = true;
            if (str.toUpperCase().endsWith("L")) {
                cArr = str.substring(0, str.length() - 1).toCharArray();
            } else {
                cArr = str.toCharArray();
            }
            int i2 = 10;
            if (cArr[0] == '-') {
                i = 1;
            } else {
                z = false;
            }
            if (cArr[i] == '0') {
                i++;
                if (i == cArr.length) {
                    return 0;
                }
                if (cArr[i] == 'x' || cArr[i] == 'X') {
                    i2 = 16;
                    i++;
                } else if (Character.digit(cArr[i], 8) >= 0) {
                    i2 = 8;
                }
            }
            long j = Long.MAX_VALUE / ((long) (i2 / 2));
            long j2 = 0;
            while (i < cArr.length) {
                int digit = Character.digit(cArr[i], i2);
                if (digit >= 0) {
                    long j3 = ((long) i2) * j2;
                    if (j2 > j) {
                        throw new NumberFormatException(str + " cannot fit into a long");
                    } else if (j3 >= 0 || j3 < ((long) (-digit))) {
                        j2 = ((long) digit) + j3;
                        i++;
                    } else {
                        throw new NumberFormatException(str + " cannot fit into a long");
                    }
                } else {
                    throw new NumberFormatException("The string contains an invalid digit - '" + cArr[i] + "'");
                }
            }
            if (!z || j2 == Long.MIN_VALUE) {
                return j2;
            }
            if (j2 >= 0) {
                return j2 * -1;
            }
            throw new NumberFormatException(str + " cannot fit into a long");
        } else {
            throw new NumberFormatException("string is blank");
        }
    }
}
package org.comboship.android;

import java.io.*;
import java.nio.file.*;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;

/** Copies a selected N64 ROM, normalizes byte order, and never writes the source. */
public final class RomImporter {
    private static final long MAX_SIZE = 128L * 1024 * 1024;
    private RomImporter() {}
    public static String importRom(InputStream input, File destination) throws IOException {
        return importRom(input, destination, 1024L * 1024);
    }
    static String importRom(InputStream input, File destination, long minimumSize) throws IOException {
        if (input == null) throw new IOException("The document provider did not return a stream");
        final File parent = destination.getCanonicalFile().getParentFile();
        if (parent == null || !parent.isDirectory()) throw new IOException("Data directory is missing");
        final Path temp = Files.createTempFile(parent.toPath(), ".rom-import-", ".tmp");
        boolean committed = false;
        try {
            MessageDigest digest = MessageDigest.getInstance("SHA-256");
            long total = 0;
            byte[] header = new byte[4];
            readFully(input, header, 0, 4);
            int order;
            int magic = ((header[0] & 255) << 24) | ((header[1] & 255) << 16)
                    | ((header[2] & 255) << 8) | (header[3] & 255);
            if (magic == 0x80371240) order = 0;
            else if (magic == 0x37804012) order = 1;
            else if (magic == 0x40123780) order = 2;
            else throw new IOException("Not a raw N64 ROM. Select a .z64, .v64, or .n64 file, not a ZIP.");
            try (FileOutputStream output = new FileOutputStream(temp.toFile())) {
                normalize(header, 4, order); output.write(header); digest.update(header); total = 4;
                byte[] buffer = new byte[64 * 1024]; int buffered = 0;
                while (true) {
                    int n = input.read(buffer, buffered, buffer.length - buffered);
                    if (n == -1) {
                        if (buffered % 4 != 0) throw new IOException("Truncated ROM: incomplete 32-bit word");
                        normalize(buffer, buffered, order); output.write(buffer, 0, buffered);
                        digest.update(buffer, 0, buffered); total += buffered; break;
                    }
                    if (n == 0) {
                        int b = input.read();
                        if (b == -1) throw new EOFException("Unexpected end of ROM stream");
                        buffer[buffered++] = (byte)b;
                    } else buffered += n;
                    if (total + buffered > MAX_SIZE) throw new IOException("ROM exceeds the 128 MiB import limit");
                    if (buffered == buffer.length) {
                        normalize(buffer, buffered, order); output.write(buffer); digest.update(buffer);
                        total += buffered; buffered = 0;
                    }
                }
                if (total < minimumSize || total > MAX_SIZE) throw new IOException("ROM has an unexpected size");
                output.flush(); output.getFD().sync();
            }
            Files.move(temp, destination.toPath(), StandardCopyOption.ATOMIC_MOVE, StandardCopyOption.REPLACE_EXISTING);
            committed = true; return hex(digest.digest());
        } catch (NoSuchAlgorithmException impossible) {throw new IOException("SHA-256 is unavailable", impossible);}
        finally {if (!committed) Files.deleteIfExists(temp);}
    }
    private static void readFully(InputStream input, byte[] buffer, int offset, int length) throws IOException {
        int done = 0;
        while (done < length) {
            int n = input.read(buffer, offset + done, length - done);
            if (n < 0) throw new EOFException("ROM header is incomplete");
            if (n == 0) {
                int b = input.read(); if (b < 0) throw new EOFException("ROM header is incomplete");
                buffer[offset + done++] = (byte)b;
            } else done += n;
        }
    }
    private static void normalize(byte[] data, int size, int order) {
        if (order == 1) for (int i = 0; i < size; i += 2) {byte t = data[i]; data[i] = data[i + 1]; data[i + 1] = t;}
        if (order == 2) for (int i = 0; i < size; i += 4) {
            byte t = data[i]; data[i] = data[i + 3]; data[i + 3] = t;
            t = data[i + 1]; data[i + 1] = data[i + 2]; data[i + 2] = t;
        }
    }
    static String hex(byte[] value) {
        StringBuilder result = new StringBuilder(value.length * 2);
        for (byte b : value) result.append(String.format(java.util.Locale.ROOT, "%02x", b & 255));
        return result.toString();
    }
}

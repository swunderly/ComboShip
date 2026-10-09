package org.comboship.android;
import java.io.*;
import java.nio.file.*;
import java.security.MessageDigest;
import java.util.*;
public final class RomImporterTest {
    private static int count;
    private static void check(boolean condition, String message) {if (!condition) throw new AssertionError(message); ++count;}
    private static byte[] encoded(byte[] canonical, int order) {
        byte[] data = canonical.clone();
        for (int i = 0; i < data.length; i += 4) {
            if (order == 1) {
                byte t = data[i]; data[i] = data[i+1]; data[i+1] = t;
                t = data[i+2]; data[i+2] = data[i+3]; data[i+3] = t;
            } else if (order == 2) {
                byte t = data[i]; data[i] = data[i+3]; data[i+3] = t;
                t = data[i+1]; data[i+1] = data[i+2]; data[i+2] = t;
            }
        }
        return data;
    }
    private static InputStream chopped(byte[] data, int chunk) {
        return new ByteArrayInputStream(data) {
            @Override public synchronized int read(byte[] out, int offset, int length) {return super.read(out, offset, Math.min(length, chunk));}
        };
    }
    public static void main(String[] args) throws Exception {
        Path root = Files.createTempDirectory("combo-java-test-");
        File destination = root.resolve("oot.z64").toFile();
        byte[] canonical = new byte[131088];new Random(473).nextBytes(canonical);
        canonical[0]=(byte)0x80; canonical[1]=0x37; canonical[2]=0x12; canonical[3]=0x40;
        String expectedHash = RomImporter.hex(MessageDigest.getInstance("SHA-256").digest(canonical));
        try {
            for (int order=0; order<3; order++) for (int chunk : new int[]{1,3,7,65536}) {
                try (InputStream in = chopped(encoded(canonical,order),chunk)) {
                    String hash = RomImporter.importRom(in,destination,4);
                    check(hash.equals(expectedHash),"normalized hash mismatch");
                    check(Arrays.equals(canonical,Files.readAllBytes(destination.toPath())),"byte-order mismatch");
                }
            }
            for (byte[] invalid : new byte[][]{new byte[0],new byte[]{1,2,3},new byte[]{1,2,3,4},Arrays.copyOf(canonical,canonical.length-1)}) {
                boolean rejected=false;
                try { RomImporter.importRom(new ByteArrayInputStream(invalid),destination,4); } catch (IOException expected) {rejected=true;}
                check(rejected,"invalid ROM accepted");
                check(Arrays.equals(canonical,Files.readAllBytes(destination.toPath())),"failed import replaced old ROM");
            }
            InputStream failing = new FilterInputStream(new ByteArrayInputStream(canonical)) {
                int calls;
                @Override public int read(byte[] out,int offset,int length) throws IOException {
                    if (++calls > 1) throw new IOException("simulated provider failure");return super.read(out,offset,length);
                }
            };
            boolean failed=false;
            try { RomImporter.importRom(failing,destination,4); } catch (IOException expected) {failed=true;}
            check(failed,"read failure not reported");
            check(Arrays.equals(canonical,Files.readAllBytes(destination.toPath())),"read failure damaged old ROM");
            try (var files=Files.list(root)) {check(files.noneMatch(p->p.getFileName().toString().startsWith(".rom-import-")),"temporary files leaked");}
            try (DataLock held = new DataLock(root.toFile())) {
                boolean busy=false;
                try (DataLock unexpected = new DataLock(root.toFile())) {} catch (IOException expected) {busy=true;}
                check(busy,"overlapping data lock accepted");
            }
            try (DataLock reacquired = new DataLock(root.toFile())) {check(true,"lock release");}
            System.out.println("PASS: "+count+" Java import and lock assertions");
        } finally {
            try (var paths=Files.walk(root)) {for (Path p:paths.sorted(Comparator.reverseOrder()).toList()) Files.delete(p);}
        }
    }
}

package org.comboship.android;
import java.io.*;
import java.nio.file.*;
import java.util.*;
public final class StorageCopyTest {
    private static int checks;
    private static void check(boolean value,String message){if(!value)throw new AssertionError(message);checks++;}
    public static void main(String[] args)throws Exception {
        Path base=Files.createTempDirectory("combo-storage-test-");
        try {
            Path source=Files.createDirectory(base.resolve("internal"));
            Files.createDirectories(source.resolve("saves/nested"));
            byte[] save=new byte[131101];new Random(239).nextBytes(save);
            Files.write(source.resolve("saves/nested/slot1.json"),save);
            Files.writeString(source.resolve(".runtime.lock"),"held");
            Files.writeString(source.resolve(".running"),"old session");
            Path card=base.resolve("card");StorageCopy.prepare(source.toFile(),card.toFile());
            check(Arrays.equals(save,Files.readAllBytes(card.resolve("saves/nested/slot1.json"))),"Save changed during copy");
            check(Arrays.equals(save,Files.readAllBytes(source.resolve("saves/nested/slot1.json"))),"Original save changed");
            check(!Files.exists(card.resolve(".runtime.lock")) && !Files.exists(card.resolve(".running")),"Session markers migrated");
            boolean rejected=false;
            try{StorageCopy.prepare(source.toFile(),card.toFile());}catch(IOException expected){rejected=true;}
            check(rejected,"Existing destination was overwritten");
            rejected=false;
            try{StorageCopy.prepare(source.toFile(),source.resolve("child").toFile());}catch(IOException expected){rejected=true;}
            check(rejected,"Recursive destination accepted");
            Path empty=Files.createDirectory(base.resolve("empty"));
            StorageCopy.prepare(source.toFile(),empty.toFile());
            check(Arrays.equals(save,Files.readAllBytes(empty.resolve("saves/nested/slot1.json"))),"An empty user-created folder could not receive data");
            Path failed=base.resolve("failed-finalizer");rejected=false;
            try{StorageCopy.prepare(source.toFile(),failed.toFile(),staging->{throw new IOException("Injected settings failure");});}
            catch(IOException expected){rejected=true;}
            check(rejected && !Files.exists(failed),"Failed settings migration published a destination");
            check(Arrays.equals(save,Files.readAllBytes(source.resolve("saves/nested/slot1.json"))),"Failed migration damaged source");
            Path contested=Files.createDirectory(base.resolve("contested"));rejected=false;
            try{StorageCopy.prepare(source.toFile(),contested.toFile(),staging->Files.writeString(contested.resolve("new-save"),"new data"));}
            catch(IOException expected){rejected=true;}
            check(rejected && Files.readString(contested.resolve("new-save")).equals("new data"),"A file added during migration was overwritten");
            if(!System.getProperty("os.name").startsWith("Windows")) {
                Files.createSymbolicLink(source.resolve("escape"),base);
                rejected=false;
                try{StorageCopy.prepare(source.toFile(),base.resolve("failed").toFile());}catch(IOException expected){rejected=true;}
                check(rejected && !Files.exists(base.resolve("failed")),"Symlink or failed copy published");
                try(var files=Files.list(base)){check(files.noneMatch(p->p.getFileName().toString().startsWith(".comboship-copy-")),"Staging data leaked");}
            }
            System.out.println("PASS: "+checks+" storage copy assertions");
        }finally {
            try(var files=Files.walk(base)){for(Path p:files.sorted(Comparator.reverseOrder()).toList())Files.delete(p);}
        }
    }
}

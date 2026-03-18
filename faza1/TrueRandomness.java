import java.security.SecureRandom;

public class TrueRandomness {
    public static final int NumBytes = 16;
    private static boolean alreadyUsed = false;

    public TrueRandomness() {
    }

    public static byte[] get() {
        assert !alreadyUsed;

        byte[] ret = new byte[16];
        SecureRandom sr = new SecureRandom();
        sr.nextBytes(ret);
        alreadyUsed = true;
        return ret;
    }
}

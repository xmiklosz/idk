import java.math.BigInteger;

public class RSAKeyPair {
    private static final BigInteger E = new BigInteger("65537");
    private final RSAKey publicKey;
    private final RSAKey privateKey;
    private BigInteger p;
    private BigInteger q;

    public RSAKeyPair(PRGen rand, int numBits) {
        if (rand == null) {
            throw new NullPointerException();
        } else if (numBits <= 0) {
            throw new IllegalArgumentException("numBits must be positive");
        } else {
            BigInteger one = new BigInteger("1");

            BigInteger N;
            BigInteger secretMod;
            do {
                this.p = BigInteger.probablePrime(numBits, rand);
                this.q = BigInteger.probablePrime(numBits, rand);
                N = this.p.multiply(this.q);
                secretMod = this.p.subtract(one).multiply(this.q.subtract(one));
            } while(N.compareTo(E) <= 0 || !E.gcd(secretMod).equals(one));

            BigInteger D = E.modInverse(secretMod);
            this.publicKey = new RSAKey(E, N);
            this.privateKey = new RSAKey(D, N);
        }
    }

    public RSAKey getPublicKey() {
        return this.publicKey;
    }

    public RSAKey getPrivateKey() {
        return this.privateKey;
    }

    public BigInteger[] getPrimes() {
        BigInteger[] ret = new BigInteger[2];
        ret[0] = this.p;
        ret[1] = this.q;
        return ret;
    }
}

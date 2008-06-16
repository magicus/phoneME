/* 
 * An abstract factory for creating Keyword instances.
 * This factory is used to make the Input class independent 
 * of the concrete class KeywordExt. 
 */
public abstract class KeywordFactory {
    /**
     * Creates a new Keyword.
     */
    abstract public Keyword create_keyword(byte[] allchars, int allchars_length, byte[] rest);
}


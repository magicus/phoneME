/** 
 * An instance of this class is a keyword, as specified in the input file.  
 */
public class Keyword {
    /** Constructor.  */
    Keyword(byte[] allchars, int allchars_length, byte[] rest) {
        _allchars = allchars;
        _allchars_length = allchars_length;
        _rest = rest;
    }

    /* Data members defined immediately by the input file.  */
  
    /** The keyword as a string, possibly containing NUL bytes.  */
    public byte[] _allchars;
    public int _allchars_length;

    /** Additional stuff seen on the same line of the input file.  */
    public byte[] _rest;

    /** Line number of this keyword in the input file.  */
    public int _lineno;
}

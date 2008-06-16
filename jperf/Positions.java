/** 
 * This class denotes a set of byte positions, used to access a keyword.  
 */
public final class Positions {
    /** Denotes the last char of a keyword, depending on the keyword's length. */
    public static final int LASTCHAR = -1;

    /** 
     * Maximum key position specifiable by the user, 1-based.
     * Note that MAX_KEY_POS-1 must fit into the element type of _positions[],
     * below.  
     */
    public static final int MAX_KEY_POS = 255;

    /** 
     * Maximum possible size.  Since duplicates are eliminated and the possible
     * 0-based positions are -1 .. MAX_KEY_POS-1, this is:  
     */
    public static final int MAX_SIZE = MAX_KEY_POS + 1;

    
    /* 
     * Constructors.  
     */
    public Positions() {
        _useall = false;
        _size = 0;
    }

    public Positions(int pos1) {
        _useall = false;
        _size = 1;
        _positions[0] = pos1;
    }

    public Positions (int pos1, int pos2) {
        _useall = false;        
        _positions[0] = pos1;
        _positions[1] = pos2;
    }

    public Positions (Positions src) {
        _useall = src._useall;
        _size = src._size;
        System.arraycopy(src._positions, 0, _positions, 0, _size);
    }
    

    /* 
     * Accessors.  
     */
    public boolean is_useall() {
        return _useall;
    }

    public int elementAt(int index) {
        return _positions[index];
    }

    public int get_size() {
        return _size;
    }


    /* 
     * Write access.  
     */
    void set_useall(boolean useall) {
        _useall = useall;
        if (useall) {
            /* The positions are 0, 1, ..., MAX_KEY_POS-1, in descending order.  */
            _size = MAX_KEY_POS;
            int idx = 0;
            for (int i = MAX_KEY_POS - 1; i >= 0; i--) {
                _positions[idx++] = i;
            }
        }
    }

    void set_size(int size) {
        _size = size;        
    }
    

    /** 
     * Sorts the array in reverse order.
     * Returns true if there are no duplicates, false otherwise.  
     */
    public boolean sort() {
        if (_useall) {
            return true;
        }

        /* Bubble sort.  */
        boolean duplicate_free = true;
        int[] base = _positions;
        int len = _size;

        for (int i = 1; i < len; i++) {
            int j;
            int tmp;

            for (j = i, tmp = base[j]; j > 0 && tmp >= base[j - 1]; j--) {
                if ((base[j] = base[j - 1]) == tmp) /* oh no, a duplicate!!! */ {
                    duplicate_free = false;
                }

                base[j] = tmp;
            }
        }
        
        return duplicate_free;
    }



    /* Set operations.  Assumes the array is in reverse order. */

    public boolean contains(int pos) {
        int count = _size;
        int idx = _size - 1;

        for (; count > 0; idx--, count--) {
            if (_positions[idx] == pos) {
                return true;
            }
            if (_positions[idx] > pos) {
                break;
            }
        }

        return false;
    }

    public void add(int pos) {
        set_useall (false);

        int count = _size;

        if (count == MAX_SIZE) {
            System.err.println("Positions::add internal error: overflow");
            System.exit(1);
        }

        int idx = _size - 1;

        for (; count > 0; idx--, count--) {
            if (_positions[idx] == pos) {
                System.err.println("Positions::add internal error: duplicate");
                System.exit(1);
            }

            if (_positions[idx] > pos) {
                break;
            }

            _positions[idx + 1] = _positions[idx];
        }

        _positions[idx + 1] = pos;
        _size++;
    }

    public void remove(int pos) {
        set_useall (false);

        int count = _size;
        if (count > 0) {
            int idx = _size - 1;

            if (_positions[idx] == pos) {
                _size--;
                return;
            }

            if (_positions[idx] < pos) {
                int prev = _positions[idx];

                for (;;) {
                    idx--;
                    count--;
                    if (count == 0) { 
                        break;
                    }

                    if (_positions[idx] == pos) {
                        _positions[idx] = prev;
                        _size--;

                        return;
                    }

                    if (_positions[idx] > pos) {
                        break;
                    }

                    int curr = _positions[idx];
                    _positions[idx] = prev;
                    prev = curr;
                }
            }
        }

        System.err.println("Positions::remove internal error: not found");
        System.exit(1);
    }
    

    /**
     * Output in external syntax. 
     */
    public void print() {
        if (_useall) {
            System.out.print("*");
        } else {
            boolean first = true;
            boolean seen_LASTCHAR = false;
            int count = _size;
            int idx = _size - 1;

            for (; count > 0; idx--) {
                count--;
                if (_positions[idx] == LASTCHAR) {
                    seen_LASTCHAR = true; 
                } else {
                    if (!first) {
                        System.out.print(",");
                    }

                    System.out.print(_positions[idx] + 1);
                    if (count > 0 && _positions[idx - 1] == _positions[idx] + 1) {
                        System.out.print("-");
                        do {
                            idx--;
                            count--;
                        } while (count > 0 &&  _positions[idx - 1] == _positions[idx] + 1);

                        System.out.print(_positions[idx] + 1);
                    }

                    first = false;
                }
            }

            if (seen_LASTCHAR) {
                if (!first) {
                    System.out.print(",");
                }

                System.out.print("$");
            }
        }
    }
    
    
    /*
     * Forward iterator 
     */
    public class PositionIterator {
        private PositionIterator() {
            _index = 0;
        }

        /** Initializes an iterator through POSITIONS, ignoring positions >= maxlen.  */
        private PositionIterator(int maxlen) {
            if (_useall) {
                _index = (maxlen <= MAX_KEY_POS ? MAX_KEY_POS - maxlen : 0);
            } else {
                int index;
                for (index = 0; index < _size && _positions[index] >= maxlen; index++)
                    ;
                _index = index;
            }
            
        }

        /** 
         * Copy constructor.
         */
        public PositionIterator(PositionIterator src) {
            _index = src._index;
        }

        /** End of iteration marker.  */
        public final static int EOS = -2;

        /** Retrieves the next position, or EOS past the end.  */
        public int next() {
            return (_index < _size ? _positions[_index++] : EOS);            
        }

        /** 
         * Returns the number of remaining positions, i.e. how often next() will
         * return a value != EOS. 
         */
        public int remaining() {
            return _size - _index;            
        }

        private int _index;
    }    


    /** Creates an iterator, returning the positions in descending order. */
    PositionIterator iterator() {
        return new PositionIterator();
    }

    /** 
     * Creates an iterator, returning the positions in descending order, 
     * that apply to strings of length <= maxlen.  
     */
    PositionIterator iterator(int maxlen) {
        return new PositionIterator(maxlen);
    }


    /** 
     * This class denotes an iterator in reverse direction through a set of 
     * byte positions.  
     */

    public class PositionReverseIterator {
        /** 
         * Initializes an iterator through POSITIONS.
         */
        private PositionReverseIterator() {
            _index = _size;
            _minindex = 0;
        }

        /**
         * Initializes an iterator through POSITIONS, ignoring positions >= maxlen.  
         */
        private PositionReverseIterator(int maxlen) {
            _index = _size;

            if (_useall) {
                _minindex = (maxlen <= MAX_KEY_POS ? MAX_KEY_POS - maxlen : 0); 
            } else {
                int index;
                for (index = 0; index < _size && _positions[index] >= maxlen; index++)
                    ;
                _minindex = index;
            }
        }

        /** 
         * Copy constructor. 
         */
        public PositionReverseIterator(PositionReverseIterator src) {
            _index = src._index;
            _minindex = src._minindex; 
        }

        /** End of iteration marker. */
        public final static int EOS = -2;

        /** Retrieves the next position, or EOS past the end.  */
        public int next() {
            return (_index > _minindex ? _positions[--_index] : EOS);
        }

        /**
         * Returns the number of remaining positions, i.e. how often next() will
         * return a value != EOS.
         */
        public int remaining() {
            return _index - _minindex;
        }

        private int _index;
        private int _minindex;
    }

    /** 
     * Creates an iterator, returning the positions in ascending order.
     */
    public PositionReverseIterator reviterator() {
        return new PositionReverseIterator();
    }

    /**
     * Creates an iterator, returning the positions in ascending order,
     * that apply to strings of length <= maxlen.  
     */
    public PositionReverseIterator reviterator(int maxlen) {
        return new PositionReverseIterator(maxlen);
    }
    


    /** The special case denoted by '*'.  */
    private boolean _useall;

    /** Number of positions.  */
    private int _size;

    /**
     * Array of positions.  0 for the first char, 1 for the second char etc.,
     * LASTCHAR for the last char.  
     */
    private int[] _positions = new int[MAX_SIZE];
}

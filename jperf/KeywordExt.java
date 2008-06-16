/* 
 * A keyword, in the context of a given keyposition list.  
 */
public class KeywordExt extends Keyword {

    /** Constructor.  */
    public KeywordExt(byte[] allchars, int allchars_length, byte[] rest) {
        super(allchars, allchars_length, rest);

        _final_index = -1;
    }

   
    /* Data members depending on the keyposition list.  */

    /** 
     * The selected characters that participate for the hash function,
     * selected according to the keyposition list, as a canonically reordered
     * multiset.
     */
    public int[] _selchars;
    public int _selchars_length;

    /**
     * Chained list of keywords having the same _selchars and
     * - if !option[NOLENGTH] - also the same _allchars_length.
     * Note that these duplicates are not members of the main keyword list.
     */
    public KeywordExt _duplicate_link;



    /* Methods depending on the keyposition list.  */

    /** 
     * Initializes selchars and selchars_length, without reordering.
     */
    public void init_selchars_tuple(Positions positions, int[] alpha_unify) {
        init_selchars_low(positions, alpha_unify, null);       
    }

    /**
     * Initializes selchars and selchars_length, with reordering.
     */
    public void init_selchars_multiset(Positions positions, int[] alpha_unify, int[] alpha_inc) {
        int[] selchars = init_selchars_low(positions, alpha_unify, alpha_inc);

        /* Sort the selchars elements alphabetically.  */
        sort_char_set(selchars, _selchars_length);
    }

    /** 
     * Deletes selchars 
     */
    public void delete_selchars() {
        _selchars = null;
    }


    /* Data members used by the algorithm.  */

    /** Hash value for the keyword.  */
    public int _hash_value; 


    /* Data members used by the output routines.  */
    public int _final_index;


    /**
     * Sort a small set of 'int', base[0..len-1], in place. 
     */
    private static void sort_char_set(int[] base, int len) {
        /* Bubble sort is sufficient here.  */
        for (int i = 1; i < len; i++) {
            int j;
            int tmp;

            for (j = i, tmp = base[j]; j > 0 && tmp < base[j - 1]; j--) {
                base[j] = base[j - 1];
            }

            base[j] = tmp;
        }
    }
    

    /* Initializes selchars and selchars_length.

       General idea:
         The hash function will be computed as
             asso_values[allchars[key_pos[0]]] +
             asso_values[allchars[key_pos[1]]] + ...
         We compute selchars as the multiset
             { allchars[key_pos[0]], allchars[key_pos[1]], ... }
         so that the hash function becomes
             asso_values[selchars[0]] + asso_values[selchars[1]] + ...
       Furthermore we sort the selchars array, to ease detection of duplicates
       later.

        More in detail: The arguments alpha_unify (used for case-insensitive
        hash functions) and alpha_inc (used to disambiguate permutations)
        apply slight modifications. The hash function will be computed as
            sum (j=0,1,...: k = key_pos[j]:
                    asso_values[alpha_unify[allchars[k]+alpha_inc[k]]])
            + (allchars_length if !option[NOLENGTH], 0 otherwise).
        We compute selchars as the multiset
            { alpha_unify[allchars[k]+alpha_inc[k]] : j=0,1,..., k = key_pos[j] }
        so that the hash function becomes
            asso_values[selchars[0]] + asso_values[selchars[1]] + ...
            + (allchars_length if !option[NOLENGTH], 0 otherwise).
    */
    private int[] init_selchars_low(Positions positions, int[] alpha_unify, int[] alpha_inc) {
        /* Iterate through the list of positions, initializing selchars */
        Positions.PositionIterator iter = positions.iterator(_allchars_length);

        int[] key_set = new int[iter.remaining()];
        int idx = 0;

        for (int i; (i = iter.next()) != Positions.PositionIterator.EOS; ) {
            int c = -1;
            if (i == Positions.LASTCHAR) {
                /* Special notation for last KEY position, i.e. '$'. */
                byte ch = _allchars[_allchars_length - 1];
                c = (int)ch & 0xFF;
            } else if (i < _allchars_length) {
                /* Within range of KEY length, so we'll keep it.  */
                byte ch = _allchars[i];
                c = (int)ch & 0xFF;
                
                if (alpha_inc != null) {
                    c += alpha_inc[i];
                }
            } else {
                /* Out of range of KEY length, the iterator should not have produced this.  */
                System.err.println("KeyWordExt: out of range of KEY length");
                System.exit(1);
            }

            if (alpha_unify != null) {
                c = alpha_unify[c];
            }
            
            key_set[idx++] = c;
        }

        _selchars = key_set;
        _selchars_length = key_set.length;

        return key_set;
    }
}


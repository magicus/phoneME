/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package javax.microedition.lcdui;





/**
 *
 * @author root
 */
public class FileSelector extends Screen {
    
    
    /**
     * This constant value indicates that the purpose of the file
     * selector is to locate a directory.
     */
    public static final int DIRECTORY = 2;
    
     /**
     * This constant value indicates that the purpose of the file
     * selector is to locate a file from which to read.
     */
    public static final int LOAD = 0;
    
     /**
     * This constant value indicates that the purpose of the file
     * selector is to locate a file to which to write.
     */
    public static final int SAVE = 1;
    
     /**
      * A Command delivered to a listener to indicate  that the FileSelector
      * has been requested to be dismissed without selecting any file.
      */
     
    public static final Command CANCEL_COMMAND = 
            new Command("Cancel", Command.CANCEL, 1);
    
    /**
      * A Command delivered to a command listener to indicate  that the file
      * has been selected from FileSelector.
      */
     
    public static final Command OK_COMMAND = 
            new Command("Select", Command.OK, 0);
    
        
    // constructors //

    /**
     * Creates a new, empty <code>FileSelector</code>, specifying its title
     * and the type. 
     * @param title the screen's title (see {@link Displayable Displayable})
     * @param mode one of <code>DIRECTORY</code>, <code>LOAD</code>,
     * or <code>SAVE</code>
     * @throws IllegalArgumentException if <code>mode</code> is not
     * one of
     * <code>DIRECTORY</code>,
     * <code>LOAD</code>, or <code>SAVE</code> 
     */
    
    public FileSelector(String title, int mode) {
        
        super(title);
                
        if (!(mode == DIRECTORY   ||
              mode == LOAD  ||
              mode == SAVE)) {
            throw new IllegalArgumentException();
        }
        String[] stringElements = listRoots();
        
        Image[] imageElements = createIcons(stringElements);        
       
        synchronized (Display.LCDUILock) {
           
            Item[] items;
            
            cg = new ChoiceGroup(null, Choice.EXCLUSIVE,
                                 stringElements, imageElements);
            cg.lSetOwner(this);
            
            if (mode == FileSelector.DIRECTORY || mode == FileSelector.SAVE) {
                items = new Item[2];
            
                TextField tf = new TextField("File Name","",10,TextField.ANY);
                tf.lSetOwner(this);            
                items[0] = tf;
                
                items[1] = cg;
            } else {
                items = new Item[1];
                items[0] = cg;
            }
            
            displayableLF = fileSelectorLF = LFFactory.getFactory().
                    getFileSelectorLF(this,items);            
            
            addCommandImpl(OK_COMMAND);                                    
            addCommandImpl(CANCEL_COMMAND);            
            
           
             
            // Note that we have to call super.setCommandListener since
            // this.setCommandListener will set application's command 
            // listener
            super.setCommandListener(internalListener);          
        }            
         
    }    
    
    
    /**
     * FileSelector has two fixed, built-in Commands: OK_COMMAND and 
     * CANCEL_COMMAND. New Commands are not allowed on FileSelector, so this
     * method will always throw IllegalArgumentsException whenever it is called.
     * 
     * @param cmd - the Command
     */
    public void addCommand(Command cmd) {
       throw new IllegalStateException();       
    }
    
    /**
     * Gets the directory of this file selector. The directory and file names
     * passed to and returned by the FileSelector are formatted as fully qualified
     * absolute file URLs [RFC 1738].
     * 
     * @return the directory of this file selector, potentially null or invalid
     */
    public String getDirectory() {
        return this.currentDirectory;    
    }
    
    /**
     * Gets the selected file of this file dialog. If the user selected CANCEL,
     * the returned file is null. The directory and file names
     * passed to and returned by the FileSelector are formatted as fully qualified
     * absolute file URLs [RFC 1738].
     * 
     * @return the currently selected file of this file selector, or null if none
     * is selected
     */
    public String getFile() {
        return this.currentFile;
    }
    
    /**
     * Gets the file extensions set for this file selector. The extension strings
     * are file system specific: for example "txt" or "xml".
     * 
     * @return the currently set file extensions for this file selector
     */
    public String[] getFilterExtension() {
        return this.extensions;
    }
    
    /**
     * Indicates whether this file dialog box is for loading from a file,
     * for saving to a file, or for finding a directory.
     * 
     * @return the mode of this FileSelector, either FileSelector.LOAD,
     * FileSelector.SAVE or FileSelector.DIRECTORY     * 
     */
    public int getMode() {
        return this.currentMode;
    }
    
    /**
     * Sets the directory of this file selector to be the specified directory.
     * Specifying a null or invalid directory implies an implementation-defined
     * default. This default will not be realized, however, until the user has
     * selected a file. Until this point, getDirectory() will return the value
     * passed into this method. Specifying "" as the directory is exactly 
     * equivalent to specifying null as the directory. The directory and
     * file names passed to and returned by the FileSelector are formatted as
     * fully qualified absolute file URLs [RFC 1738].
     *  
     * @param dir the specified directory
     */
    public void setDirectory(String dir) {
        this.currentDirectory = dir;
    }
    
    /**
     * Sets the selected file for this file selector to be the specified file.
     * This file becomes the default file if it is set before the file selector
     * is first set to visible.
     * 
     * @param file the file being set
     */
    public void setFile(String file) {
        this.currentFile = file;
    }
    
    /**
     * Sets the file extensions for this file selector to show only the files
     * with the set extensions. The extension string are file system specific:
     * for example "txt" or "xml".
     * 
     * @param extensions the file extensions being set
     */
    public void setFilterExtensions(String[] extensions) {
        this.extensions = extensions;
    }
    
    /**
     * Sets the mode of the file selector.
     * 
     * @param mode the mode for this file selector, either FileSelector.LOAD,
     * FileSelector.SAVE or FileSelector.DIRECTORY.
     * 
     */
    public void setMode(int mode) {
         if (!(mode == DIRECTORY   ||
              mode == LOAD  ||
              mode == SAVE)) {
            throw new IllegalArgumentException();
        } else {
             this.currentMode = mode;
        }
    }
     /**
     * The same as {@link Displayable#setCommandListener} but with the 
     * following additional semantics.  If the listener parameter is
     * <code>null</code>, the <em>default listener</em> is restored.
     * See <a href="#commands">Commands and Listeners</a> for the definition 
     * of the behavior of the default listener.
     *
     * @param l the new listener, or <code>null</code>
     */
    public void setCommandListener(CommandListener l) {
        synchronized (Display.LCDUILock) {
            userCommandListener = l;
        }
    }

  
    
    /**
     * Current FileSelector directory.
     */
    private String currentDirectory;
    
    /**
     * Current FileSelector file.
     */
    private String currentFile;
    
    /**
     * File extensions set for this file selector.
     */
    private String[] extensions;
    
    /**
     * Mode of the file selector
     */
    private int currentMode;
    
    /**
     * Index of ChoiceGroup focused element.
     */
    protected int cgFocusIndex;
    
    /**
     * ChoiceGroup which represents tree of file system.
     */
    private ChoiceGroup cg;
    
    /**
     * Look & Feel object associated with this FileSelector
     */
    private FormLF fileSelectorLF;
    
    /**
     * The application's command listener
     */
    private CommandListener userCommandListener;
    
     
// *****************************************************
//  Private members
// *****************************************************
           
    /**
     * Gets list of all roots of device.
     * 
     * @return massive of all roots of device
     */
    private String[] listRoots() {
        
        
        // Code for getting roots here
        
        
        //Tets code
        if (currentDirectory == null) {
            currentDirectory = "file:///root1";
            String[] stringElements = 
                    new String[] {"...","dsa","msa","fdsf","fds","dsa","msa"};           
            return stringElements;    
        } else {
            return null;
        }
        
    }
    
    /**
     * Create special image icons for file and directory.
     * 
     * @param stringElements massive of current files and directories.
     * @return
     */
    private Image[] createIcons(String[] stringElements) {
        return null;    
    }
    
    /**
     * Go down of file system tree. 
     * 
     * @param dir directory absolute name
     * 
     * @return massive of directory content
     */
    private String[] getNextDirectoryContents(String dir) {
        
        // Code for moving to next directory here
        
        //Test Code
        System.out.println("go next dir="+dir);
        currentDirectory = dir;
       
        return new String[] {};
    }
    
    /**
     * Go up of file system tree. 
     * 
     * @param dir directory absolute name
     * 
     * @return massive of directory content
     */
    private String[] getPreviousDirectoryContents(String dir) {
        
        // Code for moving to previous directory here
        
        //Test Code
        
        System.out.println("go previous dir="+dir);
        currentDirectory = dir;
        
        return new String[] {};
    }
    
    /**
     * Checks is path its a path to file or to directory.
     * 
     * @param path path to file system source
     * 
     * @return true if file
     */
    private boolean isFile(String path) {
        
        // Code for checking here
                        
        return false;        
    }
        
    /**
     * Special CommandListener instance to handle execution of
     * the default "OK" and "Cancel" Command
     */
    private CommandListener internalListener = new CommandListener() {
        /**
         * Handle the execution of the given Command and Displayable.
         * Caller of this function should hold calloutLock and catch Throwable.
	 *
         * @param c The Command to execute
         * @param s The Displayable from which the Command originated
         */
        public void commandAction(Command c, Displayable s) {
	   
            CommandListener listenerToCall = null;

	    synchronized (Display.LCDUILock) {
		                
                if (userCommandListener != null) {
                    
                    if (c == FileSelector.OK_COMMAND) {
                        
                        setFile(cg.getString(cgFocusIndex));
                        setDirectory(currentDirectory);
                        
                    } else if (c == FileSelector.CANCEL_COMMAND) {
                        
                        setFile(null);
                        setDirectory(null);
                    }
		     // To be called outside LCDUILock
		    listenerToCall = userCommandListener;
		} 
	    } // synchronized

            // SYNC NOTE: We are protected by the calloutLock obtained
            // previously. (either in Display or the timeout task)
            // We do not need to re-acquire the calloutLock when
            // calling the application's command listener.
	    if (listenerToCall != null) {
		listenerToCall.commandAction(c, s);
	    }
        }
        
    };
    
      
    /** itemStateListener that has to be notified of any state changes */
    private ItemStateListener itemStateListener = new ItemStateListener() {

        public void itemStateChanged(Item item) {
            if (item instanceof ChoiceGroup) {
                String name = cg.getString(cg.getSelectedIndex());
                
                if (!name.equals( "...")) {
                    if (!isFile(name)) {
                        getNextDirectoryContents(currentDirectory+
                            "/"+name);    
                    }                    
                } else {
                    getPreviousDirectoryContents(currentDirectory.substring(
                            0,currentDirectory.lastIndexOf('/')));
                }
                    
                
            }
        }
        
    };
     

    /**
     * Used by the event handler to notify the ItemStateListener
     * of a change in the given item
     *
     * @param item the Item which state was changed
     */
    protected void uCallItemStateChanged(Item item) {
        // get a copy of the object reference to ItemStateListener        
        ItemStateListener isl = itemStateListener;
        if (isl == null || item == null) {
            return;
        }

        // Protect from any unexpected application exceptions
        try {
            // SYNC NOTE: We lock on calloutLock around any calls
            // into application code
            synchronized (Display.calloutLock) {
                isl.itemStateChanged(item);
            }
        } catch (Throwable thr) {
            Display.handleThrowable(thr);
        }
    }    

}

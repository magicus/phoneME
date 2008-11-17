/*
 * @(#)ClassisSubclassOf.java	1.10 06/10/10
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.  
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER  
 *   
 * This program is free software; you can redistribute it and/or  
 * modify it under the terms of the GNU General Public License version  
 * 2 only, as published by the Free Software Foundation.   
 *   
 * This program is distributed in the hope that it will be useful, but  
 * WITHOUT ANY WARRANTY; without even the implied warranty of  
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU  
 * General Public License version 2 for more details (a copy is  
 * included at /legal/license.txt).   
 *   
 * You should have received a copy of the GNU General Public License  
 * version 2 along with this work; if not, write to the Free Software  
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  
 * 02110-1301 USA   
 *   
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa  
 * Clara, CA 95054 or visit www.sun.com if you need additional  
 * information or have any questions. 
 *
 */

/* This test case is created to force a C stack overflow in 
 * CVMimplementsInterface(). It first loads all super classes in the
 * class hierarchy in a way that will cause them not to link, and
 * then it uses instanceof to force C recursion in CVMimplementsInterface(). 
 */

import java.io.PrintStream;
public class ClassisSubclassOf implements Runnable {

    public static void main(String args[]) {
	Thread t = new Thread(new ClassisSubclassOf());
	t.start();
    } 
 
    public void run() {
        try {
	    Class [] argTypes = new Class[300];
            /* Load all 295 classes  and Some_Interface in a non-recursive 
	     * way and without causing them to link.
             */
	    for (int i=0; i < 295; i++) {
		argTypes[i] = Class.forName("dl" + String.valueOf(i), false,
					    getClass().getClassLoader());
		System.out.println("Load class " + "dl" + String.valueOf(i));
	    }  
 	    Class b = Class.forName("Some_Interface", false,
					    getClass().getClassLoader());
	    System.out.println("Load interface " + "Some_Interface" );

            /* Create a dl294 array. By using an array, we can avoid causing
	     * the class hierarchy to be linked.
             */
	    dl294 [][] n = new dl294[1][2];

	    /* 
	     * Using instaceof here will cause deep recursion in 
	     * CVMimplementsInterface. Note that recursion will only happen
	     * if the class hierachry is still unlinked, which is why
	     * are using array types. 
	     */ 
            if (! (n instanceof Some_Interface[][]) ) 
                System.out.print("x is not instanceof Some_Interface[][]\n" +
                    "Test INAFFECTIVE: no stack overflow. Try reducing the\n" +
                    "stack size by using -Xss32k. If you still get this\n" +
		    "message, then you are running on a platform that has \n" + 
		    "less stack usage than sparc, and the interface hierarchy\n" +
                    "of the test is not deep enough to cause a StackOverflowError.\n");
	} catch (StackOverflowError s) {
 	    s.printStackTrace();
	    System.out.println("Test PASSED: StackOverflowError thrown.");
	} catch (Throwable e) {
	    System.out.println("Test FAILED: " + e + " thrown.");
	}
    }
}

interface  dl0 {}
interface  dl1 extends dl0 {}
interface  dl2 extends dl1 {}
interface  dl3 extends dl2 {}
interface  dl4 extends dl3 {}
interface  dl5 extends dl4 {}
interface  dl6 extends dl5 {}
interface  dl7 extends dl6 {}
interface  dl8 extends dl7 {}
interface  dl9 extends dl8 {}
interface  dl10 extends dl9 {}
interface  dl11 extends dl10 {}
interface  dl12 extends dl11 {}
interface  dl13 extends dl12 {}
interface  dl14 extends dl13 {}
interface  dl15 extends dl14 {}
interface  dl16 extends dl15 {}
interface  dl17 extends dl16 {}
interface  dl18 extends dl17 {}
interface  dl19 extends dl18 {}
interface  dl20 extends dl19 {}
interface  dl21 extends dl20 {}
interface  dl22 extends dl21 {}
interface  dl23 extends dl22 {}
interface  dl24 extends dl23 {}
interface  dl25 extends dl24 {}
interface  dl26 extends dl25 {}
interface  dl27 extends dl26 {}
interface  dl28 extends dl27 {}
interface  dl29 extends dl28 {}
interface  dl30 extends dl29 {}
interface  dl31 extends dl30 {}
interface  dl32 extends dl31 {}
interface  dl33 extends dl32 {}
interface  dl34 extends dl33 {}
interface  dl35 extends dl34 {}
interface  dl36 extends dl35 {}
interface  dl37 extends dl36 {}
interface  dl38 extends dl37 {}
interface  dl39 extends dl38 {}
interface  dl40 extends dl39 {}
interface  dl41 extends dl40 {}
interface  dl42 extends dl41 {}
interface  dl43 extends dl42 {}
interface  dl44 extends dl43 {}
interface  dl45 extends dl44 {}
interface  dl46 extends dl45 {}
interface  dl47 extends dl46 {}
interface  dl48 extends dl47 {}
interface  dl49 extends dl48 {}
interface  dl50 extends dl49 {}
interface  dl51 extends dl50 {}
interface  dl52 extends dl51 {}
interface  dl53 extends dl52 {}
interface  dl54 extends dl53 {}
interface  dl55 extends dl54 {}
interface  dl56 extends dl55 {}
interface  dl57 extends dl56 {}
interface  dl58 extends dl57 {}
interface  dl59 extends dl58 {}
interface  dl60 extends dl59 {}
interface  dl61 extends dl60 {}
interface  dl62 extends dl61 {}
interface  dl63 extends dl62 {}
interface  dl64 extends dl63 {}
interface  dl65 extends dl64 {}
interface  dl66 extends dl65 {}
interface  dl67 extends dl66 {}
interface  dl68 extends dl67 {}
interface  dl69 extends dl68 {}
interface  dl70 extends dl69 {}
interface  dl71 extends dl70 {}
interface  dl72 extends dl71 {}
interface  dl73 extends dl72 {}
interface  dl74 extends dl73 {}
interface  dl75 extends dl74 {}
interface  dl76 extends dl75 {}
interface  dl77 extends dl76 {}
interface  dl78 extends dl77 {}
interface  dl79 extends dl78 {}
interface  dl80 extends dl79 {}
interface  dl81 extends dl80 {}
interface  dl82 extends dl81 {}
interface  dl83 extends dl82 {}
interface  dl84 extends dl83 {}
interface  dl85 extends dl84 {}
interface  dl86 extends dl85 {}
interface  dl87 extends dl86 {}
interface  dl88 extends dl87 {}
interface  dl89 extends dl88 {}
interface  dl90 extends dl89 {}
interface  dl91 extends dl90 {}
interface  dl92 extends dl91 {}
interface  dl93 extends dl92 {}
interface  dl94 extends dl93 {}
interface  dl95 extends dl94 {}
interface  dl96 extends dl95 {}
interface  dl97 extends dl96 {}
interface  dl98 extends dl97 {}
interface  dl99 extends dl98 {}
interface  dl100 extends dl99 {}

interface  dl101 extends dl100 {}
interface  dl102 extends dl101 {}
interface  dl103 extends dl102 {}
interface  dl104 extends dl103 {}
interface  dl105 extends dl104 {}
interface  dl106 extends dl105 {}
interface  dl107 extends dl106 {}
interface  dl108 extends dl107 {}
interface  dl109 extends dl108 {}
interface  dl110 extends dl109 {}
interface  dl111 extends dl110 {}
interface  dl112 extends dl111 {}
interface  dl113 extends dl112 {}
interface  dl114 extends dl113 {}
interface  dl115 extends dl114 {}
interface  dl116 extends dl115 {}
interface  dl117 extends dl116 {}
interface  dl118 extends dl117 {}
interface  dl119 extends dl118 {}
interface  dl120 extends dl119 {}
interface  dl121 extends dl120 {}
interface  dl122 extends dl121 {}
interface  dl123 extends dl122 {}
interface  dl124 extends dl123 {}
interface  dl125 extends dl124 {}
interface  dl126 extends dl125 {}
interface  dl127 extends dl126 {}
interface  dl128 extends dl127 {}
interface  dl129 extends dl128 {}
interface  dl130 extends dl129 {}
interface  dl131 extends dl130 {}
interface  dl132 extends dl131 {}
interface  dl133 extends dl132 {}
interface  dl134 extends dl133 {}
interface  dl135 extends dl134 {}
interface  dl136 extends dl135 {}
interface  dl137 extends dl136 {}
interface  dl138 extends dl137 {}
interface  dl139 extends dl138 {}
interface  dl140 extends dl139 {}

interface  dl141 extends dl140 {}
interface  dl142 extends dl141 {}
interface  dl143 extends dl142 {}
interface  dl144 extends dl143 {}
interface  dl145 extends dl144 {}
interface  dl146 extends dl145 {}
interface  dl147 extends dl146 {}
interface  dl148 extends dl147 {}
interface  dl149 extends dl148 {}
interface  dl150 extends dl149 {}
interface  dl151 extends dl150 {}
interface  dl152 extends dl151 {}
interface  dl153 extends dl152 {}
interface  dl154 extends dl153 {}
interface  dl155 extends dl154 {}
interface  dl156 extends dl155 {}
interface  dl157 extends dl156 {}
interface  dl158 extends dl157 {}
interface  dl159 extends dl158 {}
interface  dl160 extends dl159 {}
interface  dl161 extends dl160 {}
interface  dl162 extends dl161 {}
interface  dl163 extends dl162 {}
interface  dl164 extends dl163 {}
interface  dl165 extends dl164 {}
interface  dl166 extends dl165 {}
interface  dl167 extends dl166 {}
interface  dl168 extends dl167 {}
interface  dl169 extends dl168 {}
interface  dl170 extends dl169 {}
interface  dl171 extends dl170 {}
interface  dl172 extends dl171 {}
interface  dl173 extends dl172 {}
interface  dl174 extends dl173 {}
interface  dl175 extends dl174 {}
interface  dl176 extends dl175 {}
interface  dl177 extends dl176 {}
interface  dl178 extends dl177 {}
interface  dl179 extends dl178 {}
interface  dl180 extends dl179 {}
interface  dl181 extends dl180 {}
interface  dl182 extends dl181 {}
interface  dl183 extends dl182 {}
interface  dl184 extends dl183 {}
interface  dl185 extends dl184 {}
interface  dl186 extends dl185 {}
interface  dl187 extends dl186 {}
interface  dl188 extends dl187 {}
interface  dl189 extends dl188 {}
interface  dl190 extends dl189 {}
interface  dl191 extends dl190 {}
interface  dl192 extends dl191 {}
interface  dl193 extends dl192 {}
interface  dl194 extends dl193 {}
interface  dl195 extends dl194 {}
interface  dl196 extends dl195 {}
interface  dl197 extends dl196 {}
interface  dl198 extends dl197 {}
interface  dl199 extends dl198 {}
interface  dl200 extends dl199 {}

interface  dl201 extends dl200 {}
interface  dl202 extends dl201 {}
interface  dl203 extends dl202 {}
interface  dl204 extends dl203 {}
interface  dl205 extends dl204 {}
interface  dl206 extends dl205 {}
interface  dl207 extends dl206 {}
interface  dl208 extends dl207 {}
interface  dl209 extends dl208 {}
interface  dl210 extends dl209 {}
interface  dl211 extends dl210 {}
interface  dl212 extends dl211 {}
interface  dl213 extends dl212 {}
interface  dl214 extends dl213 {}
interface  dl215 extends dl214 {}
interface  dl216 extends dl215 {}
interface  dl217 extends dl216 {}
interface  dl218 extends dl217 {}
interface  dl219 extends dl218 {}
interface  dl220 extends dl219 {}
interface  dl221 extends dl220 {}
interface  dl222 extends dl221 {}
interface  dl223 extends dl222 {}
interface  dl224 extends dl223 {}
interface  dl225 extends dl224 {}
interface  dl226 extends dl225 {}
interface  dl227 extends dl226 {}
interface  dl228 extends dl227 {}
interface  dl229 extends dl228 {}
interface  dl230 extends dl229 {}
interface  dl231 extends dl230 {}
interface  dl232 extends dl231 {}
interface  dl233 extends dl232 {}
interface  dl234 extends dl233 {}
interface  dl235 extends dl234 {}
interface  dl236 extends dl235 {}
interface  dl237 extends dl236 {}
interface  dl238 extends dl237 {}
interface  dl239 extends dl238 {}
interface  dl240 extends dl239 {}
interface  dl241 extends dl240 {}
interface  dl242 extends dl241 {}
interface  dl243 extends dl242 {}
interface  dl244 extends dl243 {}
interface  dl245 extends dl244 {}
interface  dl246 extends dl245 {}
interface  dl247 extends dl246 {}
interface  dl248 extends dl247 {}
interface  dl249 extends dl248 {}
interface  dl250 extends dl249 {}
interface  dl251 extends dl250 {}
interface  dl252 extends dl251 {}
interface  dl253 extends dl252 {}
interface  dl254 extends dl253 {}
interface  dl255 extends dl254 {}
interface  dl256 extends dl255 {}
interface  dl257 extends dl256 {}
interface  dl258 extends dl257 {}
interface  dl259 extends dl258 {}
interface  dl260 extends dl259 {}
interface  dl261 extends dl260 {}
interface  dl262 extends dl261 {}
interface  dl263 extends dl262 {}
interface  dl264 extends dl263 {}
interface  dl265 extends dl264 {}
interface  dl266 extends dl265 {}
interface  dl267 extends dl266 {}
interface  dl268 extends dl267 {}
interface  dl269 extends dl268 {}
interface  dl270 extends dl269 {}
interface  dl271 extends dl270 {}
interface  dl272 extends dl271 {}
interface  dl273 extends dl272 {}
interface  dl274 extends dl273 {}
interface  dl275 extends dl274 {}
interface  dl276 extends dl275 {}
interface  dl277 extends dl276 {}
interface  dl278 extends dl277 {}
interface  dl279 extends dl278 {}
interface  dl280 extends dl279 {}
interface  dl281 extends dl280 {}
interface  dl282 extends dl281 {}
interface  dl283 extends dl282 {}
interface  dl284 extends dl283 {}
interface  dl285 extends dl284 {}
interface  dl286 extends dl285 {}
interface  dl287 extends dl286 {}
interface  dl288 extends dl287 {}
interface  dl289 extends dl288 {}
interface  dl290 extends dl289 {}
interface  dl291 extends dl290 {}
interface  dl292 extends dl291 {}
interface  dl293 extends dl292 {}
class  dl294 implements dl293 {}
interface  Some_Interface extends dl293 {}

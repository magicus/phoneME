/*
 * @(#)ClassLink.java	1.8 06/10/10
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

/* This test case is created to check C stack overflow in ClassLink
 * routine that loaded all super classes in the class hierarchy before
 * linking classes through making a new instance of class cl99.
 */
import java.io.PrintStream;

public class ClassLink implements Runnable {

    public static void main(String args[]) {
	Thread t = new Thread(new ClassLink());
	t.start();
    } 
 
    public void run() {
        try {
	    Class [] argTypes = new Class[300];
	    /* Load all 300 classes in a non-recursive way and without
	     * causing them to link.
	     */
	    for (int i=0; i < 300; i++) {
		argTypes[i] = Class.forName("cl" + String.valueOf(i), false,
					    getClass().getClassLoader()); 
		System.out.println("Load class" + "cl" + String.valueOf(i));
	    }
	    /* Make all the classes link. This will involve deep recursion,
	     * possibly causing a StackOverflowError error.
	     */
            Object n = new cl299();
	    System.out.println("Test INAFFECTIVE: no stack overflow." +
			       " Try reducing stack size.");
	} catch (StackOverflowError s) {
	    s.printStackTrace();
	    System.out.println("Test PASSED: StackOverflowError thrown.");
	} catch (Throwable e) {
	    System.out.println("Test FAILED: " + e + " thrown.");
	}
    }
}

class  cl0 {}
class  cl1 extends cl0 {}
class  cl2 extends cl1 {}
class  cl3 extends cl2 {}
class  cl4 extends cl3 {}
class  cl5 extends cl4 {}
class  cl6 extends cl5 {}
class  cl7 extends cl6 {}
class  cl8 extends cl7 {}
class  cl9 extends cl8 {}
class  cl10 extends cl9 {}
class  cl11 extends cl10 {}
class  cl12 extends cl11 {}
class  cl13 extends cl12 {}
class  cl14 extends cl13 {}
class  cl15 extends cl14 {}
class  cl16 extends cl15 {}
class  cl17 extends cl16 {}
class  cl18 extends cl17 {}
class  cl19 extends cl18 {}
class  cl20 extends cl19 {}
class  cl21 extends cl20 {}
class  cl22 extends cl21 {}
class  cl23 extends cl22 {}
class  cl24 extends cl23 {}
class  cl25 extends cl24 {}
class  cl26 extends cl25 {}
class  cl27 extends cl26 {}
class  cl28 extends cl27 {}
class  cl29 extends cl28 {}
class  cl30 extends cl29 {}
class  cl31 extends cl30 {}
class  cl32 extends cl31 {}
class  cl33 extends cl32 {}
class  cl34 extends cl33 {}
class  cl35 extends cl34 {}
class  cl36 extends cl35 {}
class  cl37 extends cl36 {}
class  cl38 extends cl37 {}
class  cl39 extends cl38 {}
class  cl40 extends cl39 {}
class  cl41 extends cl40 {}
class  cl42 extends cl41 {}
class  cl43 extends cl42 {}
class  cl44 extends cl43 {}
class  cl45 extends cl44 {}
class  cl46 extends cl45 {}
class  cl47 extends cl46 {}
class  cl48 extends cl47 {}
class  cl49 extends cl48 {}
class  cl50 extends cl49 {}
class  cl51 extends cl50 {}
class  cl52 extends cl51 {}
class  cl53 extends cl52 {}
class  cl54 extends cl53 {}
class  cl55 extends cl54 {}
class  cl56 extends cl55 {}
class  cl57 extends cl56 {}
class  cl58 extends cl57 {}
class  cl59 extends cl58 {}
class  cl60 extends cl59 {}
class  cl61 extends cl60 {}
class  cl62 extends cl61 {}
class  cl63 extends cl62 {}
class  cl64 extends cl63 {}
class  cl65 extends cl64 {}
class  cl66 extends cl65 {}
class  cl67 extends cl66 {}
class  cl68 extends cl67 {}
class  cl69 extends cl68 {}
class  cl70 extends cl69 {}
class  cl71 extends cl70 {}
class  cl72 extends cl71 {}
class  cl73 extends cl72 {}
class  cl74 extends cl73 {}
class  cl75 extends cl74 {}
class  cl76 extends cl75 {}
class  cl77 extends cl76 {}
class  cl78 extends cl77 {}
class  cl79 extends cl78 {}
class  cl80 extends cl79 {}
class  cl81 extends cl80 {}
class  cl82 extends cl81 {}
class  cl83 extends cl82 {}
class  cl84 extends cl83 {}
class  cl85 extends cl84 {}
class  cl86 extends cl85 {}
class  cl87 extends cl86 {}
class  cl88 extends cl87 {}
class  cl89 extends cl88 {}
class  cl90 extends cl89 {}
class  cl91 extends cl90 {}
class  cl92 extends cl91 {}
class  cl93 extends cl92 {}
class  cl94 extends cl93 {}
class  cl95 extends cl94 {}
class  cl96 extends cl95 {}
class  cl97 extends cl96 {}
class  cl98 extends cl97 {}
class  cl99 extends cl98 {}
class  cl100 extends cl99 {}

class  cl101 extends cl100 {}
class  cl102 extends cl101 {}
class  cl103 extends cl102 {}
class  cl104 extends cl103 {}
class  cl105 extends cl104 {}
class  cl106 extends cl105 {}
class  cl107 extends cl106 {}
class  cl108 extends cl107 {}
class  cl109 extends cl108 {}
class  cl110 extends cl109 {}
class  cl111 extends cl110 {}
class  cl112 extends cl111 {}
class  cl113 extends cl112 {}
class  cl114 extends cl113 {}
class  cl115 extends cl114 {}
class  cl116 extends cl115 {}
class  cl117 extends cl116 {}
class  cl118 extends cl117 {}
class  cl119 extends cl118 {}
class  cl120 extends cl119 {}
class  cl121 extends cl120 {}
class  cl122 extends cl121 {}
class  cl123 extends cl122 {}
class  cl124 extends cl123 {}
class  cl125 extends cl124 {}
class  cl126 extends cl125 {}
class  cl127 extends cl126 {}
class  cl128 extends cl127 {}
class  cl129 extends cl128 {}
class  cl130 extends cl129 {}
class  cl131 extends cl130 {}
class  cl132 extends cl131 {}
class  cl133 extends cl132 {}
class  cl134 extends cl133 {}
class  cl135 extends cl134 {}
class  cl136 extends cl135 {}
class  cl137 extends cl136 {}
class  cl138 extends cl137 {}
class  cl139 extends cl138 {}
class  cl140 extends cl139 {}
class  cl141 extends cl140 {}
class  cl142 extends cl141 {}
class  cl143 extends cl142 {}
class  cl144 extends cl143 {}
class  cl145 extends cl144 {}
class  cl146 extends cl145 {}
class  cl147 extends cl146 {}
class  cl148 extends cl147 {}
class  cl149 extends cl148 {}
class  cl150 extends cl149 {}
class  cl151 extends cl150 {}
class  cl152 extends cl151 {}
class  cl153 extends cl152 {}
class  cl154 extends cl153 {}
class  cl155 extends cl154 {}
class  cl156 extends cl155 {}
class  cl157 extends cl156 {}
class  cl158 extends cl157 {}
class  cl159 extends cl158 {}
class  cl160 extends cl159 {}
class  cl161 extends cl160 {}
class  cl162 extends cl161 {}
class  cl163 extends cl162 {}
class  cl164 extends cl163 {}
class  cl165 extends cl164 {}
class  cl166 extends cl165 {}
class  cl167 extends cl166 {}
class  cl168 extends cl167 {}
class  cl169 extends cl168 {}
class  cl170 extends cl169 {}
class  cl171 extends cl170 {}
class  cl172 extends cl171 {}
class  cl173 extends cl172 {}
class  cl174 extends cl173 {}
class  cl175 extends cl174 {}
class  cl176 extends cl175 {}
class  cl177 extends cl176 {}
class  cl178 extends cl177 {}
class  cl179 extends cl178 {}
class  cl180 extends cl179 {}
class  cl181 extends cl180 {}
class  cl182 extends cl181 {}
class  cl183 extends cl182 {}
class  cl184 extends cl183 {}
class  cl185 extends cl184 {}
class  cl186 extends cl185 {}
class  cl187 extends cl186 {}
class  cl188 extends cl187 {}
class  cl189 extends cl188 {}
class  cl190 extends cl189 {}
class  cl191 extends cl190 {}
class  cl192 extends cl191 {}
class  cl193 extends cl192 {}
class  cl194 extends cl193 {}
class  cl195 extends cl194 {}
class  cl196 extends cl195 {}
class  cl197 extends cl196 {}
class  cl198 extends cl197 {}
class  cl199 extends cl198 {}
class  cl200 extends cl199 {}

class  cl201 extends cl200 {}
class  cl202 extends cl201 {}
class  cl203 extends cl202 {}
class  cl204 extends cl203 {}
class  cl205 extends cl204 {}
class  cl206 extends cl205 {}
class  cl207 extends cl206 {}
class  cl208 extends cl207 {}
class  cl209 extends cl208 {}
class  cl210 extends cl209 {}
class  cl211 extends cl210 {}
class  cl212 extends cl211 {}
class  cl213 extends cl212 {}
class  cl214 extends cl213 {}
class  cl215 extends cl214 {}
class  cl216 extends cl215 {}
class  cl217 extends cl216 {}
class  cl218 extends cl217 {}
class  cl219 extends cl218 {}
class  cl220 extends cl219 {}
class  cl221 extends cl220 {}
class  cl222 extends cl221 {}
class  cl223 extends cl222 {}
class  cl224 extends cl223 {}
class  cl225 extends cl224 {}
class  cl226 extends cl225 {}
class  cl227 extends cl226 {}
class  cl228 extends cl227 {}
class  cl229 extends cl228 {}
class  cl230 extends cl229 {}
class  cl231 extends cl230 {}
class  cl232 extends cl231 {}
class  cl233 extends cl232 {}
class  cl234 extends cl233 {}
class  cl235 extends cl234 {}
class  cl236 extends cl235 {}
class  cl237 extends cl236 {}
class  cl238 extends cl237 {}
class  cl239 extends cl238 {}
class  cl240 extends cl239 {}
class  cl241 extends cl240 {}
class  cl242 extends cl241 {}
class  cl243 extends cl242 {}
class  cl244 extends cl243 {}
class  cl245 extends cl244 {}
class  cl246 extends cl245 {}
class  cl247 extends cl246 {}
class  cl248 extends cl247 {}
class  cl249 extends cl248 {}
class  cl250 extends cl249 {}
class  cl251 extends cl250 {}
class  cl252 extends cl251 {}
class  cl253 extends cl252 {}
class  cl254 extends cl253 {}
class  cl255 extends cl254 {}
class  cl256 extends cl255 {}
class  cl257 extends cl256 {}
class  cl258 extends cl257 {}
class  cl259 extends cl258 {}
class  cl260 extends cl259 {}
class  cl261 extends cl260 {}
class  cl262 extends cl261 {}
class  cl263 extends cl262 {}
class  cl264 extends cl263 {}
class  cl265 extends cl264 {}
class  cl266 extends cl265 {}
class  cl267 extends cl266 {}
class  cl268 extends cl267 {}
class  cl269 extends cl268 {}
class  cl270 extends cl269 {}
class  cl271 extends cl270 {}
class  cl272 extends cl271 {}
class  cl273 extends cl272 {}
class  cl274 extends cl273 {}
class  cl275 extends cl274 {}
class  cl276 extends cl275 {}
class  cl277 extends cl276 {}
class  cl278 extends cl277 {}
class  cl279 extends cl278 {}
class  cl280 extends cl279 {}
class  cl281 extends cl280 {}
class  cl282 extends cl281 {}
class  cl283 extends cl282 {}
class  cl284 extends cl283 {}
class  cl285 extends cl284 {}
class  cl286 extends cl285 {}
class  cl287 extends cl286 {}
class  cl288 extends cl287 {}
class  cl289 extends cl288 {}
class  cl290 extends cl289 {}
class  cl291 extends cl290 {}
class  cl292 extends cl291 {}
class  cl293 extends cl292 {}
class  cl294 extends cl293 {}
class  cl295 extends cl294 {}
class  cl296 extends cl295 {}
class  cl297 extends cl296 {}
class  cl298 extends cl297 {}
class  cl299 extends cl298 {}

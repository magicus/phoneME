/*
 *   
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
 */

# include "incls/_precompiled.incl"
# include "incls/_SystemDictionary.cpp.incl"

#ifndef PRODUCT
#define LAST_CLASS_LOADED_BUF_SIZE 1024
char SystemDictionary::_last_class_loaded[LAST_CLASS_LOADED_BUF_SIZE];
#endif

ReturnOop SystemDictionary::bucket_for(ObjArray *sd, juint hash_value) {
  juint len = (juint)sd->length();
  juint index;
  if (len == 64) {
    index = hash_value & (64 - 1);
  } else {
    index = hash_value % len;
  }
  return sd->obj_at(index);
}

void SystemDictionary::set_bucket_for(ObjArray *sd, juint hash_value,
                                      InstanceClass* bucket) {
  sd->obj_at_put(hash_value % sd->length(), bucket);
}

// lookup_only -- only search loaded classes, don't try to load
//                from classpath.
// check_only  -- if not found, don't throw an exception
ReturnOop SystemDictionary::find(LoaderContext *loader_ctx, 
                                 bool lookup_only, 
                                 bool check_only JVM_TRAPS) {

  ObjArray::Raw dictionary = Universe::system_dictionary();
 
  if (!dictionary.is_null()) {
    InstanceClass::Raw cl = 
      find_class_in_dictionary(&dictionary, loader_ctx, lookup_only);
    if (cl().not_null()) {
      return cl;
    }
  }
  dictionary = Universe::current_dictionary();
  if (!dictionary.is_null()) {
    InstanceClass::Raw cl = 
      find_class_in_dictionary(&dictionary, loader_ctx, lookup_only);
    if (cl().not_null()) {
      return cl;
    }
  }
  if (!check_only) {
    Throw::class_not_found(loader_ctx JVM_NO_CHECK_AT_BOTTOM);
  }
  return NULL;
}


ReturnOop SystemDictionary::find_class_in_dictionary(ObjArray *dictionary,
                                 LoaderContext *loader_ctx, 
                                 bool lookup_only) {

  juint hash_value = loader_ctx->class_name()->hash();
  for (InstanceClass::Raw cl = bucket_for(dictionary, hash_value); 
       !cl.is_null(); 
       cl = cl().next()) {
    Symbol::Raw name = cl().name();
    if (loader_ctx->class_name()->equals(&name)) {
      // class match 
      if (lookup_only) {
        // return the class immediately for lookups only
        return cl;
      } 
    } else { 
      GUARANTEE(!loader_ctx->class_name()->matches(&name), "symbols misuse!");
      // no match found
      continue;
    }
    if (cl().is_fake_class() && loader_ctx->resolve_mode() != SemiResolve) {
      continue;
    }

#ifdef USE_CLASS_LOADER
    Oop::Raw loader = cl().class_loader();
    if (!loader_ctx->class_loader->equals(&loader)) {
      continue;
    }
#endif
    return cl;
  }
  return NULL;
}


ReturnOop SystemDictionary::find_class_or_null(Symbol *class_name) {
  AllocationDisabler shouldnt_allocate_in_any_cases;
  SETUP_ERROR_CHECKER_ARG;

  LoaderContext ctx(class_name);
  InstanceClass::Raw found = find(&ctx, true, true JVM_NO_CHECK);
  GUARANTEE(!CURRENT_HAS_PENDING_EXCEPTION, "unexpected exception");
  if (found.not_null() && !found().is_fake_class()) {
    return found.obj();
  } else {
    return NULL;
  }
}

ReturnOop SystemDictionary::fetch_buffer(LoaderContext *loader_ctx JVM_TRAPS) {
  UsingFastOops fast_oops;
  FileDecoder::Fast fd = 
    ClassPathAccess::open_entry(loader_ctx->class_name(), true JVM_CHECK_0);

  if (fd.not_null()) {
    if (UseROM && 
        ClassFileParser::is_package_restricted(loader_ctx->class_name()) 
        && !(fd().flags() & SYSTEM_CLASSPATH)) {
      Throw::class_not_found(loader_ctx->class_name(), ErrorOnFailure 
                             JVM_THROW_0);
    }
    Buffer::Raw result = fd().read_completely(JVM_SINGLE_ARG_CHECK_0);
    if (result.not_null()) {
      loader_ctx->set_is_system_class(fd().flags() & SYSTEM_CLASSPATH);      
      return result;
    }
  }

#if USE_BINARY_IMAGE_LOADER
  ObjArray::Raw bad_classes = Task::current()->names_of_bad_classes();
  if (bad_classes.not_null()) {
    GUARANTEE((bad_classes().length() % 2) == 0, "Must be pairs");
    for (int i = 0; i < bad_classes().length(); i += 2) {
      Symbol::Raw name = bad_classes().obj_at(i);
      if (name.equals(loader_ctx->class_name())) {
        Symbol::Raw exception_class_name = bad_classes().obj_at(i + 1);
        String::Raw message;
        /*
         * This class had generate an error during Monet
         * conversion. The TCK requires us to throw that Error, not
         * ClassNoDefFoundException. 
         */
        Throw::allocate_and_throw(&exception_class_name, &message JVM_THROW_0);
      }
    }
  }
#endif
  
  Throw::class_not_found(loader_ctx JVM_THROW_0);
}

ReturnOop SystemDictionary::load_system_class(LoaderContext *loader_ctx JVM_TRAPS) {
  UsingFastOops fast_oops;

  ClassParserState::Fast state = 
      ClassParserState::allocate(loader_ctx->class_name() JVM_CHECK_0);
  // optimization, if we have a buffer loaded already for this class, then use it
  if (loader_ctx->buf() != NULL) {
    state().set_buffer(loader_ctx->buf());
  }
  // Initialize the class loading stack so that it contains one element
  ClassParserState::Fast stack;
  stack().push(&state);

  // Keep parsing until the class loading stack is empty
  while (!stack.is_null()) {
    UsingFastOops fast_oops_inside;
    ClassParserState::Fast cur_class = stack().top();
    Symbol::Fast class_name = cur_class().class_name();

    NOT_PRODUCT(class_name().string_copy(_last_class_loaded, LAST_CLASS_LOADED_BUF_SIZE);)
    
    FailureMode fail_mode;
    if (stack().next() == NULL) {
      fail_mode = loader_ctx->fail_mode();
    } else {
      // Always throw an error if we're loading a class which
      // loader_ctx->class_name depends on.
      fail_mode = ErrorOnFailure;
    }
    LoaderContext top_ctx(&class_name, fail_mode, loader_ctx);

    InstanceClass::Fast ic;
    if (cur_class().stage() == 0) {
      ic = find(&top_ctx, /*lookup_only=*/ false,
                /*check_only=*/ true JVM_CHECK_0);
      if (!ic.is_null()) {
        // We may come to here if we have:
        //   class A implements X,Y;
        //   interface X extends Y;
        // If we start with loading A, the stack may look like this:
        //   Y, X, Y, A
        // After we resolve the first instance of Y, the second Y
        // should be ignored.
        GUARANTEE(stack().next() != NULL, "can't the last element in stack!");
        stack().pop();
        continue;
      }
    } else {
#ifdef AZZERT
      ic = find(&top_ctx, /*lookup_only=*/ false,
                /*check_only=*/ true JVM_CHECK_0);
      GUARANTEE(ic.is_null(), 
                "Partially loaded class cannot be already inserted");
#endif
    }

    if (ic.is_null()) {
      UsingFastOops level_3;
      // Load the buffer (from class file or from JAR), if necessary
      Buffer::Fast buffer = cur_class().buffer();      
      if (buffer.is_null()) {
          buffer = fetch_buffer(&top_ctx JVM_CHECK_0);
          cur_class().set_buffer(&buffer);
          if (top_ctx.is_system_class()) {
            cur_class().set_access_flags(cur_class().access_flags() | JVM_ACC_PRELOADED);            
          }
      }

      // Parse the class at the top of the stack. If it refers to some
      // class(es) that are not yet loader, they will be added to the
      // top of the stack.
      ClassFileParser parser(&buffer, &top_ctx);
      ic = parser.parse_class(&stack JVM_CHECK_0);
    }

    GUARANTEE(cur_class.obj() != stack.obj(),
              "Some progress must have been made!");

    if (!ic.is_null()) {
      // completed the loading of one class
      insert(&top_ctx, &ic JVM_CHECK_0);
#if !ENABLE_ISOLATES
      if (!UseROM ||
          (ic.obj_field(InstanceClass::java_mirror_offset()) == NULL)) {
        ic().setup_java_mirror(JVM_SINGLE_ARG_CHECK_0);
      }
#endif
      VMEvent::class_prepare_event(&ic);
    }
  }

  return state().result();
}

void SystemDictionary::insert(LoaderContext *loader_ctx, 
                              InstanceClass* instance_class JVM_TRAPS) {
  GUARANTEE(!instance_class->is_null(), "sanity check");
  UsingFastOops fastoops;

  ObjArray::Fast dictionary;
  dictionary = Universe::current_dictionary();
#if ENABLE_ISOLATES
  GUARANTEE(dictionary.not_null(),"dictionary is null");
#endif
 
  Symbol::Raw name = instance_class->name();
  juint hash_value = name().hash();

  if (loader_ctx->resolve_mode() != SemiResolve) {
    // A fake class may have been installed. Must replace it.
    InstanceClass::Raw ic = bucket_for(&dictionary, hash_value);
    InstanceClass::Raw last = ic;

    while (ic.not_null()) {

      InstanceClass::Raw next = ic().next();
      Symbol::Raw name2 = ic().name();

      if (name2.equals(&name)) {
        GUARANTEE(ic().is_fake_class(), "duplicate must be fake");
        if (ic.equals(&last)) {
          set_bucket_for(&dictionary, hash_value, &next);
        } else {
          last().set_next(&next);
        }
        update_fake_class(instance_class, &ic JVM_CHECK);
        break;
      } else {
        last = ic;
        ic = ic().next();
      }
    }
  }
  InstanceClass::Raw next = bucket_for(&dictionary, hash_value);
  instance_class->set_next(&next);
  set_bucket_for(&dictionary, hash_value, instance_class);

#ifdef AZZERT
  if (loader_ctx != NULL) {
    InstanceClass::Raw result = find(loader_ctx JVM_CHECK);
    GUARANTEE(!result.is_null(), "check insert");
  }
#endif
}

void SystemDictionary::update_fake_class(InstanceClass *real_cls, 
                                         InstanceClass *fake_cls JVM_TRAPS)
{
  UsingFastOops fastoops;
  Universe::pop_class_id(real_cls, fake_cls);

  ObjArrayClass::Fast ac = fake_cls->array_class();
  if (ac.not_null()) {
#if ENABLE_ISOLATES
    InstanceClass::Raw element_class = ac().element_class();
    GUARANTEE(!element_class.is_null(), "Null element class");
    // ac.class_id() MUST be >= ROM::number_of_system_classes. Can't have
    // fake classes in ROM ???
    GUARANTEE(ac().class_id() >= ROM::number_of_system_classes(),
              "fake class in ROM");
    if (!(GenerateROMImage && !ENABLE_MONET) &&
        element_class().class_id() < ROM::number_of_system_classes()) {
      // This is an arrayclass whose element class is in ROM, need a TaskMirror
      TaskMirror::Raw tm = ac().task_mirror_no_check();
      if (tm().is_being_initialized_mirror()) {
        tm = TaskMirror::clinit_list_lookup(&ac);
        if (tm.is_null()) {
          ac().setup_task_mirror(0, 0, false JVM_CHECK);
        }
      }
      // Need to dance around this problem: when we created the fake class
      // we put it into the task_array_class_list with the fake class being 
      // the containing class in the TaskMirror.  So we need to remove
      // the Task Mirror from the list and create a new one
      tm = fake_cls->task_mirror_no_check();
      //      GUARANTEE(!tm.is_null(), "Task mirror is null");
      if (tm().is_being_initialized_mirror()) {
        // class is being initialized, remove fake class TaskMirror
        TaskMirror::clinit_list_remove(fake_cls);
        Universe::mirror_list()->obj_at_put(real_cls->class_id(),
                                           Universe::task_class_init_marker());
      }
      real_cls->setup_task_mirror(real_cls->static_field_size(),
                                  real_cls->vtable_length(),
                                  Universe::before_main() ? false : true
                                  JVM_CHECK);
    }
#else 
  JVM_IGNORE_TRAPS;
#endif
    ac().set_element_class(real_cls);
    real_cls->set_array_class(&ac JVM_CHECK);
  }

  ObjArray::Raw methods = real_cls->methods();
  int len = methods().length();
  int class_id = real_cls->class_id();
  for (int i=0; i<len; i++) {
    Method::Raw m = methods().obj_at(i);
    m().set_holder_id((jushort) class_id);
  }
}

ReturnOop 
SystemDictionary::resolve_array_class(LoaderContext *loader_ctx JVM_TRAPS) {
  UsingFastOops fast_oops;
  ArrayClass::Fast ac = 
    TypeSymbol::array_class_for_encoded_name(loader_ctx->class_name());
  if (ac.is_null()) {
    return NULL;
  }

  if (loader_ctx->resolve_mode() == FullResolve) {
    JavaClass::Raw klass = ac.obj();
    for (;;) {
      if (klass().is_type_array_class()) {
        // type array classes are always resolved
        break;
      } else if (klass().is_instance_class()) {
        InstanceClass::Raw ic = klass.obj();
        if (ic().is_fake_class()) {
          UsingFastOops fast_oops2;
          Symbol::Fast name = ic().name();
          LoaderContext ctx(&name, loader_ctx->fail_mode(), loader_ctx);
          SystemDictionary::resolve(&ctx JVM_CHECK_0);
        }
        break;
      } else if (klass().is_obj_array_class()) {
        ObjArrayClass::Raw oac = klass.obj();
        klass = oac().element_class();
      }
    }
  }

  return ac;
}

ReturnOop SystemDictionary::create_fake_class(LoaderContext *loader_ctx JVM_TRAPS){
  UsingFastOops fast_oops;
  InstanceClass::Fast ic = Universe::new_instance_class(0, 0, 0, 0, 
                                                        2, 4
                                                        JVM_CHECK_0);
  // set the embedded oopmap to indicate there are no static or non-static
  // oops
  {
    const int offset = ic().first_nonstatic_map_offset();
    ic().oop_map_at_put(offset, OopMapSentinel);   // a blank non-static oopmap
    ic().oop_map_at_put(offset+1, OopMapSentinel); // a blank static oopmap
  }

  ClassInfo::Fast klass_info = ic().class_info();
  klass_info().set_is_fake_class();
  klass_info().set_name(loader_ctx->class_name());
  // ROMizer uses a fake class to represent a missing class.
  // We need to fully initialize this object, so that an application with 
  // a missing class is ROMized successfully and fails with an appropriate
  // exception on run-time.
#if ENABLE_ROM_GENERATOR
  if (PostponeErrorsUntilRuntime) {
    klass_info().set_fields(Universe::empty_short_array());
    klass_info().set_methods(Universe::empty_obj_array()); 
    klass_info().set_local_interfaces(Universe::empty_short_array()); 
    // When ROMizing an application all classes should be verified as
    // soon as they are loaded or created, so that later on we can treat
    // an unverified class as a class that failed verification.
    klass_info().set_is_verified();
    // We create a non-empty constant pool, since otherwise ROMizer will fail 
    // on attempt to write an empty tag array.
    ConstantPool::Raw cp = Universe::new_constant_pool(1 JVM_CHECK_0);
    klass_info().set_constants(&cp);
  }
#endif
 
  insert(loader_ctx, &ic JVM_CHECK_0);
  return ic;
}

ReturnOop SystemDictionary::resolve(LoaderContext *loader_ctx JVM_TRAPS) {
  UsingFastOops fast_oops;
  if (!loader_ctx->class_name()->is_valid_class_name()) {
    Throw::class_not_found(loader_ctx JVM_THROW_0);
  }

  InstanceClass::Fast ic = resolve_array_class(loader_ctx JVM_CHECK_0);
  if (!ic.is_null()) {
    return ic;
  }

  // Resolve Instance Class
  ic = find(loader_ctx JVM_CHECK_0);
  if (!ic.is_null()) {
    return ic;
  }
  if (loader_ctx->resolve_mode() == SemiResolve) {
     return create_fake_class(loader_ctx JVM_NO_CHECK_AT_BOTTOM);
  }

#if ENABLE_PERFORMANCE_COUNTERS
  jlong start_time = Os::elapsed_counter();
#endif

  ic = load_system_class(loader_ctx JVM_NO_CHECK);

#if ENABLE_PERFORMANCE_COUNTERS
  jlong elapsed = Os::elapsed_counter() - start_time;

  jvm_perf_count.num_of_class_loaded++;
  jvm_perf_count.total_load_hrticks += elapsed;
  if (jvm_perf_count.max_load_hrticks < elapsed) {
      jvm_perf_count.max_load_hrticks = elapsed;
  }
#endif

  if (CURRENT_HAS_PENDING_EXCEPTION) {
    return NULL;
  } else {
    return ic;
  }
}

#ifndef PRODUCT
void LoaderContext::print_on(Stream *st) {
#if USE_DEBUG_PRINTING
  st->print("class_name   = ");
  if (_class_name) {
    _class_name->print_symbol_on(st);
    st->cr();
  } else {
    st->print_cr("NULL");
  }
  st->print("resolve_mode = %s", _resolve_mode == FullResolve ?
                                 "FullResolve" : "SemiResolve");

  st->print("fail_mode    = %s", _fail_mode == ExceptionOnFailure ?
                                 "ExceptionOnFailure" : "ErrorOnFailure");
#endif
}

#endif

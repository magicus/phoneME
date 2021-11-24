phoneME Archive
===

Background
---
This repository contains the archived source code of the old [Sun
phoneME](https://en.wikipedia.org/wiki/PhoneME) project, converted to git.

Additional links:
 * [Java ME on Wikipedia](https://en.wikipedia.org/wiki/Java_Platform,_Micro_Edition)
 * [Archive of the java.net phoneME project](https://web.archive.org/web/20170410014909/http://java.net/projects/phoneme)
 * [Archive of the java.net phoneME wiki](https://web.archive.org/web/20070824053825/http://wiki.java.net/bin/view/Mobileandembedded/WebHome)
 * [Archive of phoneme.dev.java.net](https://web.archive.org/web/20070610234355/https://phoneme.dev.java.net/)

Conversion Notes
---
The original source was published as Subversion at
https://svn.java.net/svn/phoneme~svn. This server is long defunct, but the SVN
sources were saved to Archive.org as a SVN dump at
https://archive.org/details/phoneme-svn.dump. I have taken this dump and
massaged it into a sort-of usable git repo. This was trickier than it sounds,
due to the odd layout of the Subversion repo. For details on the repo layout,
see
[PhoneMERepositoryStructure](https://web.archive.org/web/20070827161423/http://wiki.java.net/bin/view/Mobileandembedded/PhoneMERepositoryStructure?TWIKISID=7992c77b1c83a3f85f93aadb39959382).

In short, I have imported each individual component (with tags and branches) as
separate roots in the git repo. I have also imported the `legal`, `builds` and
`releases` sub-trees the same way. In general, the names of tags and branches
were unique enough to not collide, but there were a few exceptions. For these,
an arbitrary source tag/branch was chosen and the other(s) are lost. See
[missing references](#missing-references) below for a list of such tags and
branches that is not 100% correctly mapped from SVN.

Finally, to get a default view of the repo which makes the most sense, I have
created a master branch into which I have merged all the individual trunks from
the components (and `legal`, and the top level trunk with `www`). In some sense,
this is the most recent view of the original SVN repository.

I chose to put the individual components directly in the root directory (like
`cdc`), and not in a `components` directory (like `components/cdc`). While the
latter had better matched the original layout, the method I choose makes it easy
to checkout a tag from `builds` or `releases` and get a similar layout.

Conversion Methodology
---
Initially, I created a separate git repository for each component, using `git
svn` for `trunk`, `tags` and `branches`, like this:

```
git svn clone file://.../phoneME-svn --no-metadata -bcomponents/$1/branches -tcomponents/$1/tags -Tcomponents/$1/trunk $1
```

Similarly, I created repositories for `build` and `release`, but for these, I
interpreted the subdirectories as git tags:

```
git svn clone file:///.../phoneME-svn --no-metadata -t$1 $1
```

Finally, I created `legal`, and the main trunk (containing `www`) using just the
trunk import argument:

```
git svn clone file:///.../phoneME-svn --no-metadata -T$1 $1
```

For some reason I could not fully understand, `git svn` did not create proper
tags and branches, but instead only populated `.git/info/refs`. I could not
figure out a proper git way of converting these to real branches and tags, so I
ran this script for each mini-repo:

```
addtag () {
  git show-ref | grep refs/remotes/origin/tags/$1 | cut -d " " -f1 > .git/refs/tags/$1
}

addbranch () {
  git show-ref | grep refs/remotes/origin/$1 | cut -d " " -f1 > .git/refs/heads/$1
}

git show-ref | grep refs/remotes/origin/ | grep -v refs/remotes/origin/tags/ | cut -d " " -f 2 | cut -d "/" -f 4- > BRANCHNAMES
git show-ref | grep refs/remotes/origin/tags/ | cut -d " " -f 2 | cut -d "/" -f 5-  > TAGNAMES

while read p; do
  addtag $p
done < TAGNAMES

while read p; do
  addbranch $p
done < BRANCHNAMES
```

After this, I re-joined all these separate git repos into one. I did this by
starting out with the `trunk` repo, and then importing commits, tags and
branches from each individual mini-repo. I also merged the master (trunk) of
each individual mini-repo into the master of the consolidated repository,
placing it in a suitable location using `git read-tree`. In effect, this is what
I did per mini-repo:

```
git remote add $1 /.../$1
git fetch $1
git fetch $1 --tags
git merge -s ours --no-commit $1/master --allow-unrelated-histories
git read-tree --prefix=$1/ -u $1/master
git commit -m "Merge in $1/"
```

Missing References
---

Duplicated tags, where only one were kept
```
Java_ME_SDK_Mac_CLDC-Darwin-158
midp-cdc-win32-gci-1
midp-mr2-promo-b05
phoneme_advanced-mr1-rel-b06
phoneme_feature-mr1-rc-b04
sdk-tt-cf1
sdk-tt-pr1
```

Duplicated branches, where only one were kept
```
abstractions-cr-6678413
cdc-112_02
cdc-cr-6520657
cdc-proto
cldc-cr-6779573-time
code-subm-davyp-20080812
javacall-cr-6769974
javacall-mr2-cr-6517470
jc_perm_fix
jmesdk-agui-oi
jsr120-cr-6551021
jsr211-cr-6662559
leto_r1
leto_r1_mr1
leto_r1_mr1_rotation3
leto_r1_mr1-rt_l10n
midp-cdc-win32-gci
midp-cr-6714912-asyn_network_commit_fix
midp-mr2-cr-6508174-TCK-OpenRecordStore1
midp-mr2-cr-6524911
midp-mr2-ixc-fix
pmea-mr2-ea1
protocol-permissions
push-refactoring
sdk-tt-cf1
ui_service
```

Build Instructions
---

For compiling the components on a Linux AMD64 system, you need a GCC able to
emit i686 code as well as a JDK capable of emitting Java 1.4 bytecode such as
JDK6.

Additionally you need the equivalents of the following dependencies for your
distribution (the packages here are listed for Fedora 35):

* glibc-static.i686
* libstdc++-static.i686
* binutils.i686
* musl-devel.i686
* glibc-devel.i686
* gcc-c++.i686

To build the CDC VM follow these steps:

1. Change to cdc/build/linux-x86-suse.
2. Ensure the correct javac is on the $PATH, see comment above.
3. Run `make`.

You will find the `cvm` VM executable in the bin/ subfolder.

To build the CLDC VM follow these steps:

1. Change to cldc/build/linux_i386
2. Set the JVMWorkSpace environment variable to the absolute path of the cldc
   directory.
3. Set the JDK_DIR environment variable to your JDK distribution (see comment
   above).
4. Run `make ENABLE_COMPILATION_WARNINGS=true ROMIZING=false ENABLE_JNI=false`.

You will find the `cldc_vm` executable in the target/bin subfolder. Please note
that the system classpath needs to be passed into the VM together with your
application classpath. You can find it in cldc/build/classes. Currently it is
required to disable romizing and JNI due to build issues with these options
enabled. The ENABLE_COMPILATION_WARNINGS option disables the use of -Werror
which causes the build to fail with modern compilers.
`

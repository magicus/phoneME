phoneME Archive
===

Background
---
This repository contains the archived source code of the old [Sun
phoneME](https://en.wikipedia.org/wiki/PhoneME) project, converted to git.

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


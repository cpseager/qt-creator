TEMPLATE = aux

include(../../../qtcreator.pri)

LANGUAGES = cs de fr ja pl ru sl uk zh_CN zh_TW
#OPENMV-DIFF#
LANGUAGES += ar es hu it
#OPENMV-DIFF#
# *don't* re-enable these without a prior rework
BAD_LANGUAGES = hu

# var, prepend, append
defineReplace(prependAll) {
    for(a,$$1):result += $$2$${a}$$3
    return($$result)
}

XMLPATTERNS = $$shell_path($$[QT_INSTALL_BINS]/xmlpatterns)
#OPENMV-DIFF#
#LUPDATE = $$shell_path($$[QT_INSTALL_BINS]/lupdate) -locations relative -no-ui-lines -no-sort
#LRELEASE = $$shell_path($$[QT_INSTALL_BINS]/lrelease)
#LCONVERT = $$shell_path($$[QT_INSTALL_BINS]/lconvert)
#OPENMV-DIFF#
LUPDATE = $$shell_path($$[QT_HOST_BINS]/lupdate) -locations relative -no-ui-lines -no-sort
LRELEASE = $$shell_path($$[QT_HOST_BINS]/lrelease)
LCONVERT = $$shell_path($$[QT_HOST_BINS]/lconvert)
#OPENMV-DIFF#

wd = $$replace(IDE_SOURCE_TREE, /, $$QMAKE_DIR_SEP)

TRANSLATIONS = $$prependAll(LANGUAGES, $$PWD/qtcreator_,.ts)

MIME_TR_H = $$OUT_PWD/mime_tr.h
CUSTOMWIZARD_TR_H = $$OUT_PWD/customwizard_tr.h
JSONWIZARD_TR_H = $$OUT_PWD/jsonwizard_tr.h
QMLWIZARD_TR_H = $$OUT_PWD/qmlwizard_tr.h
QTQUICKWIZARD_TR_H = $$OUT_PWD/qtquickwizard_tr.h
EXTERNALTOOLS_TR_H = $$OUT_PWD/externaltools_tr.h

win32: \
    PREFIX = "file:///"
else: \
    PREFIX = "file://"

for(dir, $$list($$files($$IDE_SOURCE_TREE/src/plugins/*))):MIMETYPES_FILES += $$files($$dir/*.mimetypes.xml)
MIMETYPES_FILES = \"$$join(MIMETYPES_FILES, "|$$PREFIX", "$$PREFIX")\"

for(dir, $$list($$files($$IDE_SOURCE_TREE/share/qtcreator/templates/wizards/*, true))):CUSTOMWIZARD_FILES += $$files($$dir/wizard.xml)
CUSTOMWIZARD_FILES = \"$$join(CUSTOMWIZARD_FILES, "|$$PREFIX", "$$PREFIX")\"

for(dir, $$list($$files($$IDE_SOURCE_TREE/share/qtcreator/templates/qml/*))):QMLWIZARD_FILES += $$files($$dir/template.xml)
QMLWIZARD_FILES = \"$$join(QMLWIZARD_FILES, "|$$PREFIX", "$$PREFIX")\"

for(dir, $$list($$files($$IDE_SOURCE_TREE/share/qtcreator/templates/qtquick/*))):QTQUICKWIZARD_FILES += $$files($$dir/template.xml)
QTQUICKWIZARD_FILES = \"$$join(QTQUICKWIZARD_FILES, "|$$PREFIX", "$$PREFIX")\"

for(file, $$list($$files($$IDE_SOURCE_TREE/src/share/qtcreator/externaltools/*))):EXTERNALTOOLS_FILES += $$files($$file)
EXTERNALTOOLS_FILES = \"$$join(EXTERNALTOOLS_FILES, "|$$PREFIX", "$$PREFIX")\"

extract.commands += \
    $$XMLPATTERNS -output $$MIME_TR_H -param files=$$MIMETYPES_FILES $$PWD/extract-mimetypes.xq $$escape_expand(\\n\\t) \
    $$XMLPATTERNS -output $$CUSTOMWIZARD_TR_H -param files=$$CUSTOMWIZARD_FILES $$PWD/extract-customwizards.xq $$escape_expand(\\n\\t) \
    $$XMLPATTERNS -output $$QMLWIZARD_TR_H -param files=$$QMLWIZARD_FILES $$PWD/extract-qmlwizards.xq $$escape_expand(\\n\\t) \
    $$XMLPATTERNS -output $$QTQUICKWIZARD_TR_H -param files=$$QTQUICKWIZARD_FILES $$PWD/extract-qtquickwizards.xq $$escape_expand(\\n\\t) \
    $$XMLPATTERNS -output $$EXTERNALTOOLS_TR_H -param files=$$EXTERNALTOOLS_FILES $$PWD/extract-externaltools.xq $$escape_expand(\\n\\t) \
    $(QMAKE) -o Makefile.jsonwizard JSONWIZARD_TR_H=\"$$JSONWIZARD_TR_H\" TOP_LEVEL=\"$$IDE_SOURCE_TREE/share/qtcreator/templates/wizards\" $$PWD/jsonwizard_tr.pro
QMAKE_EXTRA_TARGETS += extract

#OPENMV-DIFF#
#plugin_sources = $$files($$IDE_SOURCE_TREE/src/plugins/*)
#plugin_sources ~= s,^$$re_escape($$IDE_SOURCE_TREE/),,g$$i_flag
#plugin_sources -= src/plugins/plugins.pro \
#    src/plugins/helloworld \ # just an example
#    # the following ones are dead
#    src/plugins/qtestlib \
#    src/plugins/snippets \
#    src/plugins/regexp
#OPENMV-DIFF#
plugin_sources = src/plugins/coreplugin \
                 src/plugins/texteditor \
                 src/plugins/openmv
#OPENMV-DIFF#
#OPENMV-DIFF#
#shared_sources = $$files($$IDE_SOURCE_TREE/src/shared/*)
#shared_sources ~= s,^$$re_escape($$IDE_SOURCE_TREE/),,g$$i_flag
#shared_sources -= \
#    src/shared/qbs
#OPENMV-DIFF#
shared_sources = src/shared/json \
                 src/shared/qtlockedfile \
                 src/shared/qtsingleapplication
#OPENMV-DIFF#
#OPENMV-DIFF#
#sources = src/app src/libs $$plugin_sources $$shared_sources share/qtcreator/qmldesigner \
#          share/qtcreator/welcomescreen share/qtcreator/welcomescreen/widgets
#OPENMV-DIFF#
sources = src/app \
          src/libs/aggregation \
          src/libs/extensionsystem \
          src/libs/utils \
          $$plugin_sources \
          $$shared_sources
#OPENMV-DIFF#

for(path, INCLUDEPATH): include_options *= -I$$shell_quote($$path)

files = $$files($$PWD/*_??.ts) $$PWD/qtcreator_untranslated.ts
for(file, files) {
    lang = $$replace(file, .*_([^/]*)\\.ts, \\1)
    v = ts-$${lang}.commands
    #OPENMV-DIFF#
    #$$v = cd $$wd && $$LUPDATE $$include_options $$sources $$MIME_TR_H $$CUSTOMWIZARD_TR_H $$JSONWIZARD_TR_H $$QMLWIZARD_TR_H $$QTQUICKWIZARD_TR_H $$EXTERNALTOOLS_TR_H -ts $$file
    #OPENMV-DIFF#
    $$v = cd $$wd && $$LUPDATE $$include_options $$sources -ts $$file && $$LCONVERT $$file -o $$replace(file, (.*_[^/]*)\\.ts, \\1).po
    #OPENMV-DIFF#
    #OPENMV-DIFF#
    #v = ts-$${lang}.depends
    #$$v = extract
    #OPENMV-DIFF#
    QMAKE_EXTRA_TARGETS += ts-$$lang
    #OPENMV-DIFF#
    v = ts-$${lang}-update.commands
    $$v = cd $$wd && $$LCONVERT -locations relative $$replace(file, (.*_[^/]*)\\.ts, \\1).po -o $$file
    QMAKE_EXTRA_TARGETS += ts-$${lang}-update
    #OPENMV-DIFF#
}
#OPENMV-DIFF#
#ts-all.commands = cd $$wd && $$LUPDATE $$include_options $$sources $$MIME_TR_H $$CUSTOMWIZARD_TR_H $$JSONWIZARD_TR_H $$QMLWIZARD_TR_H $$QTQUICKWIZARD_TR_H $$EXTERNALTOOLS_TR_H -ts $$files
#ts-all.depends = extract
#OPENMV-DIFF#
ts-all.commands = cd $$wd && $$LUPDATE $$include_options $$sources -ts $$files
for(file, files) {
    ts-all.commands += && $$LCONVERT $$file -o $$replace(file, (.*_[^/]*)\\.ts, \\1).po
}
#OPENMV-DIFF#
QMAKE_EXTRA_TARGETS += ts-all
#OPENMV-DIFF#
ts-all-update.commands = cd $$wd
for(file, files) {
    ts-all-update.commands += && $$LCONVERT -locations relative $$replace(file, (.*_[^/]*)\\.ts, \\1).po -o $$file
}
QMAKE_EXTRA_TARGETS += ts-all-update
#OPENMV-DIFF#

check-ts.commands = (cd $$replace(PWD, /, $$QMAKE_DIR_SEP) && perl check-ts.pl)
check-ts.depends = ts-all
QMAKE_EXTRA_TARGETS += check-ts

isEqual(QMAKE_DIR_SEP, /) {
    commit-ts.commands = \
        cd $$wd; \
        git add -N share/qtcreator/translations/*_??.ts && \
        for f in `git diff-files --name-only share/qtcreator/translations/*_??.ts`; do \
            $$LCONVERT -locations none -i \$\$f -o \$\$f; \
        done; \
        git add share/qtcreator/translations/*_??.ts && git commit
} else {
    commit-ts.commands = \
        cd $$wd && \
        git add -N share/qtcreator/translations/*_??.ts && \
        for /f usebackq %%f in (`git diff-files --name-only share/qtcreator/translations/*_??.ts`) do \
            $$LCONVERT -locations none -i %%f -o %%f $$escape_expand(\\n\\t) \
        cd $$wd && git add share/qtcreator/translations/*_??.ts && git commit
}
QMAKE_EXTRA_TARGETS += commit-ts

ts.commands = \
    @echo \"The \'ts\' target has been removed in favor of more fine-grained targets.\" && \
    echo \"Use \'ts-<lang>\' instead. To add a language, use \'ts-untranslated\',\" && \
    echo \"rename the file and re-run \'qmake\'.\"
QMAKE_EXTRA_TARGETS += ts

updateqm.input = TRANSLATIONS
updateqm.output = $$IDE_DATA_PATH/translations/${QMAKE_FILE_BASE}.qm
isEmpty(vcproj):updateqm.variable_out = PRE_TARGETDEPS
updateqm.commands = $$LRELEASE ${QMAKE_FILE_IN} -qm ${QMAKE_FILE_OUT}
updateqm.name = LRELEASE ${QMAKE_FILE_IN}
updateqm.CONFIG += no_link
QMAKE_EXTRA_COMPILERS += updateqm

qmfiles.files = $$prependAll(LANGUAGES, $$OUT_PWD/qtcreator_,.qm)
qmfiles.path = $$INSTALL_DATA_PATH/translations
qmfiles.CONFIG += no_check_exist
INSTALLS += qmfiles

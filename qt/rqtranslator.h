#ifndef RQTRANSLATOR_H
#define RQTRANSLATOR_H

#include <qtranslator.h>

#include "config.h"

#include <i18n.h>

class RQTranslator : public QTranslator
{
   public:

   QTranslatorMessage findMessage(const char *context,
                                  const char *sourceText,
                                  const char *comment = 0) const
   {
      QTranslatorMessage tm = QTranslatorMessage(context, _(sourceText), comment);
      tm.setTranslation(QString::fromUtf8(_(sourceText)));
      return tm;
   }
};

#endif

// vim:ts=3:sw=3:et

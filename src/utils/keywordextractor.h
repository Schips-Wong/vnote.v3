#ifndef KEYWORDEXTRACTOR_H
#define KEYWORDEXTRACTOR_H

#include <QString>
#include <QVector>

namespace vnotex {
// Extract candidate keywords (for auto tag detection) from note content.
//
// It works for both CJK and Latin text without any external segmentation library:
//  - Latin words are lower-cased and filtered by a stop-word list.
//  - CJK runs are turned into 2/3/4-gram candidates, filtered by stop characters/words.
//  - Tokens appearing in Markdown headings (lines starting with '#') get a higher weight.
// The result is sorted by weight (descending) and sub-string redundant candidates are removed.
class KeywordExtractor {
public:
  KeywordExtractor() = delete;

  struct Keyword {
    QString m_word;

    // Higher means more relevant.
    int m_weight = 0;
  };

  // Return at most @p_maxKeywords keywords, sorted by relevance.
  static QVector<Keyword> extract(const QString &p_text, int p_maxKeywords = 30);

  // Rough Markdown cleanup (remove code blocks, link URLs, inline code, HTML tags, ...).
  static QString stripMarkdown(const QString &p_text);
};
} // namespace vnotex

#endif // KEYWORDEXTRACTOR_H

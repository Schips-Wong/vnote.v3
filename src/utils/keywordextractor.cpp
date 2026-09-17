#include "keywordextractor.h"

#include <QChar>
#include <QHash>
#include <QRegularExpression>
#include <QSet>
#include <QStringList>

#include <algorithm>
#include <initializer_list>
#include <utility>

using namespace vnotex;

namespace {

inline bool isCjkChar(QChar p_char) {
  const ushort u = p_char.unicode();
  return (u >= 0x4E00 && u <= 0x9FFF) || (u >= 0x3400 && u <= 0x4DBF);
}

inline bool isWordChar(QChar p_char) {
  if (isCjkChar(p_char)) {
    return false;
  }
  return p_char.isLetterOrNumber() || p_char == QLatin1Char('+') || p_char == QLatin1Char('#');
}

inline bool isCjkToken(const QString &p_token) {
  return !p_token.isEmpty() && isCjkChar(p_token.at(0));
}

// A valid keyword must contain at least one letter or digit.
// This filters out pure-symbol tokens such as Markdown heading markers ("####").
inline bool containsLetterOrDigit(const QString &p_token) {
  for (const auto &ch : p_token) {
    if (ch.isLetterOrNumber()) {
      return true;
    }
  }
  return false;
}

// NOTE: keep this file ASCII-only (no non-ASCII literals) so that it compiles correctly
// regardless of the compiler's source/execution charset. Chinese characters are written as
// their Unicode code points instead.

const QSet<QString> &englishStopWords() {
  static const QSet<QString> words = {
      QStringLiteral("the"),      QStringLiteral("and"),      QStringLiteral("for"),
      QStringLiteral("are"),      QStringLiteral("but"),      QStringLiteral("not"),
      QStringLiteral("you"),      QStringLiteral("all"),      QStringLiteral("any"),
      QStringLiteral("can"),      QStringLiteral("had"),      QStringLiteral("her"),
      QStringLiteral("was"),      QStringLiteral("one"),      QStringLiteral("our"),
      QStringLiteral("out"),      QStringLiteral("has"),      QStringLiteral("his"),
      QStringLiteral("how"),      QStringLiteral("its"),      QStringLiteral("who"),
      QStringLiteral("did"),      QStringLiteral("yes"),      QStringLiteral("get"),
      QStringLiteral("use"),      QStringLiteral("used"),     QStringLiteral("using"),
      QStringLiteral("via"),      QStringLiteral("with"),     QStringLiteral("this"),
      QStringLiteral("that"),     QStringLiteral("these"),    QStringLiteral("those"),
      QStringLiteral("from"),     QStringLiteral("into"),     QStringLiteral("than"),
      QStringLiteral("then"),     QStringLiteral("them"),     QStringLiteral("they"),
      QStringLiteral("their"),    QStringLiteral("there"),    QStringLiteral("here"),
      QStringLiteral("when"),     QStringLiteral("where"),    QStringLiteral("which"),
      QStringLiteral("while"),    QStringLiteral("will"),     QStringLiteral("would"),
      QStringLiteral("should"),   QStringLiteral("could"),    QStringLiteral("about"),
      QStringLiteral("after"),    QStringLiteral("before"),   QStringLiteral("between"),
      QStringLiteral("because"),  QStringLiteral("been"),     QStringLiteral("being"),
      QStringLiteral("also"),     QStringLiteral("more"),     QStringLiteral("most"),
      QStringLiteral("some"),     QStringLiteral("such"),     QStringLiteral("only"),
      QStringLiteral("other"),    QStringLiteral("both"),     QStringLiteral("each"),
      QStringLiteral("very"),     QStringLiteral("just"),     QStringLiteral("make"),
      QStringLiteral("made"),     QStringLiteral("like"),     QStringLiteral("does"),
      QStringLiteral("what"),     QStringLiteral("were"),     QStringLiteral("must"),
      QStringLiteral("shall"),    QStringLiteral("may"),      QStringLiteral("might"),
      QStringLiteral("note"),     QStringLiteral("notes"),    QStringLiteral("example")};

  return words;
}

QSet<QChar> makeCharSet(std::initializer_list<ushort> p_codes) {
  QSet<QChar> set;
  for (const ushort code : p_codes) {
    set.insert(QChar(code));
  }
  return set;
}

QSet<QString> makeWordSet(std::initializer_list<std::pair<ushort, ushort>> p_pairs) {
  QSet<QString> set;
  for (const auto &pair : p_pairs) {
    set.insert(QString(QChar(pair.first)) + QChar(pair.second));
  }
  return set;
}

// Common functional Chinese characters (of, at, is, this, ...) that should not be part of a tag.
const QSet<QChar> &cjkStopChars() {
  static const QSet<QChar> chars = makeCharSet({0x7684, 0x4E86, 0x5728, 0x662F, 0x6211, 0x6709,
                                                 0x548C, 0x5C31, 0x4E0D, 0x90FD, 0x4E5F, 0x5F88,
                                                 0x5230, 0x8BF4, 0x8981, 0x53BB, 0x4F60, 0x4F1A,
                                                 0x7740, 0x6CA1, 0x81EA, 0x8FD9, 0x90A3, 0x4E3A,
                                                 0x4EE5, 0x4E8E, 0x800C, 0x4E0E, 0x53CA, 0x6216,
                                                 0x4F46, 0x8FD8, 0x88AB, 0x628A, 0x8BA9, 0x4ECE,
                                                 0x5BF9, 0x80FD, 0x53EF, 0x5C06, 0x7B49, 0x5E76,
                                                 0x5176, 0x4E4B, 0x6240, 0x5982, 0x82E5, 0x5219,
                                                 0x4E14, 0x8005, 0x4E2A, 0x4EEC, 0x4E48, 0x5462,
                                                 0x5417, 0x554A, 0x5427, 0x4EC0});
  return chars;
}

// Common Chinese bigrams/fillers that should not become tags.
const QSet<QString> &cjkStopWords() {
  static const QSet<QString> words = makeWordSet({{0x56E0, 0x4E3A}, // 因为
                                                  {0x5982, 0x679C}, // 如果
                                                  {0x5DF2, 0x7ECF}, // 已经
                                                  {0x4E00, 0x4E2A}, // 一个
                                                  {0x65F6, 0x5019}, // 时候
                                                  {0x81EA, 0x5DF1}, // 自己
                                                  {0x77E5, 0x9053}, // 知道
                                                  {0x89C9, 0x5F97}, // 觉得
                                                  {0x5E94, 0x8BE5}, // 应该
                                                  {0x9700, 0x8981}, // 需要
                                                  {0x4F7F, 0x7528}, // 使用
                                                  {0x8FDB, 0x884C}, // 进行
                                                  {0x901A, 0x8FC7}, // 通过
                                                  {0x7136, 0x540E}, // 然后
                                                  {0x73B0, 0x5728}, // 现在
                                                  {0x95EE, 0x9898}, // 问题
                                                  {0x65B9, 0x5F0F}, // 方式
                                                  {0x60C5, 0x51B5}, // 情况
                                                  {0x8FD9, 0x6837}, // 这样
                                                  {0x90A3, 0x6837}}); // 那样
  return words;
}

bool containsOnlyStopChars(const QString &p_token) {
  for (const auto &ch : p_token) {
    if (!cjkStopChars().contains(ch)) {
      return false;
    }
  }
  return true;
}

void addTokens(const QString &p_text, int p_extraWeight, QHash<QString, int> &p_counts) {
  const int n = p_text.size();
  int i = 0;
  while (i < n) {
    const QChar c = p_text.at(i);
    if (isCjkChar(c)) {
      const int start = i;
      while (i < n && isCjkChar(p_text.at(i))) {
        ++i;
      }
      const QString run = p_text.mid(start, i - start);
      for (int len = 2; len <= 4; ++len) {
        for (int k = 0; k + len <= run.size(); ++k) {
          p_counts[run.mid(k, len)] += 1 + p_extraWeight;
        }
      }
    } else if (isWordChar(c)) {
      const int start = i;
      while (i < n && isWordChar(p_text.at(i))) {
        ++i;
      }
      QString word = p_text.mid(start, i - start).toLower();

      // Strip leading symbol characters (e.g. Markdown heading markers '#').
      int lead = 0;
      while (lead < word.size() && !word.at(lead).isLetterOrNumber()) {
        ++lead;
      }
      word = word.mid(lead);

      if (word.size() >= 3 && containsLetterOrDigit(word)) {
        p_counts[word] += 1 + p_extraWeight;
      }
    } else {
      ++i;
    }
  }
}

} // namespace

QString KeywordExtractor::stripMarkdown(const QString &p_text) {
  QString text = p_text;

  // Fenced code blocks.
  text.remove(QRegularExpression(QStringLiteral("```[\\s\\S]*?```")));

  // Images: ![alt](url) -> alt.
  text.replace(QRegularExpression(QStringLiteral("!\\[([^\\]]*)\\]\\([^)]*\\)")),
               QStringLiteral("\\1"));

  // Links: [text](url) -> text.
  text.replace(QRegularExpression(QStringLiteral("\\[([^\\]]*)\\]\\([^)]*\\)")),
               QStringLiteral("\\1"));

  // Inline code.
  text.remove(QRegularExpression(QStringLiteral("`[^`]*`")));

  // HTML tags.
  text.remove(QRegularExpression(QStringLiteral("<[^>]+>")));

  // Bare URLs.
  text.remove(QRegularExpression(QStringLiteral("\\bhttps?://\\S+")));

  return text;
}

QVector<KeywordExtractor::Keyword> KeywordExtractor::extract(const QString &p_text,
                                                             int p_maxKeywords) {
  QVector<Keyword> result;
  if (p_maxKeywords <= 0) {
    return result;
  }

  const QString text = stripMarkdown(p_text);
  if (text.trimmed().isEmpty()) {
    return result;
  }

  QHash<QString, int> counts;
  addTokens(text, 0, counts);

  // Boost tokens appearing in Markdown headings.
  const auto lines = text.split(QLatin1Char('\n'));
  for (const auto &line : lines) {
    const auto trimmed = line.trimmed();
    if (trimmed.startsWith(QLatin1Char('#'))) {
      addTokens(trimmed, 2, counts);
    }
  }

  QVector<Keyword> all;
  all.reserve(counts.size());
  for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
    const QString &word = it.key();
    const int count = it.value();

    // Never emit pure-symbol tokens (e.g. Markdown heading markers "####").
    if (!containsLetterOrDigit(word)) {
      continue;
    }

    if (isCjkToken(word)) {
      if (containsOnlyStopChars(word)) {
        continue;
      }
      if (cjkStopWords().contains(word)) {
        continue;
      }
      if (count < 2) {
        continue;
      }
    } else {
      if (englishStopWords().contains(word)) {
        continue;
      }
      if (count < 2) {
        continue;
      }
    }

    Keyword kw;
    kw.m_word = word;
    kw.m_weight = count;
    all.push_back(kw);
  }

  std::sort(all.begin(), all.end(), [](const Keyword &p_a, const Keyword &p_b) {
    if (p_a.m_weight != p_b.m_weight) {
      return p_a.m_weight > p_b.m_weight;
    }
    if (p_a.m_word.size() != p_b.m_word.size()) {
      return p_a.m_word.size() > p_b.m_word.size();
    }
    return p_a.m_word < p_b.m_word;
  });

  // Drop a candidate if it is a sub-string of an already selected (higher ranked) one.
  for (const auto &kw : all) {
    bool isRedundant = false;
    for (const auto &selected : result) {
      if (selected.m_word.contains(kw.m_word)) {
        isRedundant = true;
        break;
      }
    }
    if (isRedundant) {
      continue;
    }

    result.push_back(kw);
    if (result.size() >= p_maxKeywords) {
      break;
    }
  }

  return result;
}

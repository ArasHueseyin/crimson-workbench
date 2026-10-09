//! Literal part-word search and bounded, linear-time regular expressions.
use crate::{Error, Result};
pub(crate) enum Matcher {
    All,
    Key(u32),
    Words(Vec<String>),
    Literal(String),
    Regex(regex::Regex),
}
fn fold(value: &str) -> String {
    value
        .chars()
        .filter(|c| c.is_alphanumeric())
        .flat_map(char::to_lowercase)
        .collect()
}
impl Matcher {
    pub(crate) fn new(text: &str, regex: bool) -> Result<Self> {
        if text.chars().count() > 512 {
            return Err(Error::Invalid(
                "Suchtext ist zu lang (maximal 512 Zeichen).".into(),
            ));
        }
        let text = text.trim();
        if text.is_empty() {
            return Ok(Self::All);
        }
        if regex {
            return regex::RegexBuilder::new(text)
                .case_insensitive(true)
                .multi_line(true)
                .nest_limit(64)
                .size_limit(1_000_000)
                .dfa_size_limit(2_000_000)
                .build()
                .map(Self::Regex)
                .map_err(|e| {
                    Error::Invalid(format!(
                        "Ungültiges oder nicht unterstütztes Regexmuster: {e}"
                    ))
                });
        }
        if let Ok(key) = text.parse::<u32>() {
            return Ok(Self::Key(key));
        }
        let words: Vec<_> = text
            .split_whitespace()
            .map(fold)
            .filter(|w| !w.is_empty())
            .collect();
        if words.is_empty() {
            return Ok(Self::Literal(text.to_lowercase()));
        }
        Ok(Self::Words(words))
    }
    pub(crate) fn matches(&self, key: u32, fields: &[&str]) -> bool {
        match self {
            Self::All => true,
            Self::Key(expected) => key == *expected,
            Self::Words(words) => {
                let text = fold(&fields.join("\n"));
                words.iter().all(|w| text.contains(w))
            }
            Self::Literal(text) => fields
                .iter()
                .any(|field| field.to_lowercase().contains(text)),
            Self::Regex(regex) => {
                fields.iter().any(|text| regex.is_match(text)) || regex.is_match(&key.to_string())
            }
        }
    }
}
#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn part_words_plural_punctuation_unicode_and_combined_terms() {
        for query in ["pfeil", "PFEIL", "blitz pfeil", "blitzpfeil", "Blitz-Pfeil"] {
            assert!(
                Matcher::new(query, false)
                    .unwrap()
                    .matches(1001315, &["Blitz-Pfeile", "Munition"])
            );
        }
        assert!(Matcher::new("löw", false).unwrap().matches(1, &["Löwen"]));
        assert!(Matcher::new("[", false).unwrap().matches(1, &["[Selten]"]));
        assert!(
            !Matcher::new("[", false)
                .unwrap()
                .matches(1, &["Goldbarren"])
        );
        assert!(
            !Matcher::new("pfeil holz", false)
                .unwrap()
                .matches(1, &["Blitz-Pfeile"])
        );
        assert!(
            !Matcher::new("100", false)
                .unwrap()
                .matches(1001315, &["100 Pfeile"])
        );
    }
    #[test]
    fn regex_is_optional_case_insensitive_and_rejects_invalid_patterns() {
        assert!(
            Matcher::new(r"^(Blitz|Feuer).*pfeil", true)
                .unwrap()
                .matches(1, &["Blitz-Pfeile"])
        );
        assert!(
            !Matcher::new(r"^Blitz", true)
                .unwrap()
                .matches(1, &["Feuer-Pfeile"])
        );
        assert!(Matcher::new("[", true).is_err());
        assert!(Matcher::new(r"(?<=Blitz)pfeil", true).is_err());
        assert!(Matcher::new(&"a".repeat(513), false).is_err());
        assert!(
            Matcher::new(r"^(a+)+$", true)
                .unwrap()
                .matches(1, &[&"a".repeat(10000)])
        );
        assert!(
            !Matcher::new(r"^(a+)+$", true)
                .unwrap()
                .matches(1, &[&format!("{}!", "a".repeat(10000))])
        );
    }
}

# Final manuscript insertions and reviewer response draft

## 1. Corpus-count caveat to resolve before submission

The current manuscript says the corpus contains 307 reports. The downloaded public text corpus used in the robustness run loaded 323 text files. One file appears to be a minutes file (`cosl_04_minutes_01_14_1967.txt`), but that does not fully explain the difference. Before final submission, the authors should reconcile whether the 307 figure came from a manual count of reports or whether some files in the downloaded text corpus should be excluded from the computational run.

Suggested internal note to Srijoni:

> The robustness run was completed on the downloaded CoSL text corpus, which loaded 323 text files. The current draft says 307 reports, so before final submission we should reconcile whether the original 307 count came from the manual metadata count or whether some text files in the public corpus should be excluded. I have not silently changed the manuscript count.

## 2. Revised Section 3.1: Text preprocessing

Replace the current short-word-removal explanation with:

> To enhance the quality of topic modelling, the report texts were cleaned through case normalisation, whitespace normalisation, and removal of common stopwords. We used a combination of standard stopwords and a corpus-specific stopword list for terms that occur frequently in parliamentary reports but do not help distinguish between topics. In response to concerns that preprocessing choices may influence the topic model, we also conducted robustness checks using alternative preprocessing specifications. The first reproduced the original-style preprocessing approach, including removal of tokens of three characters or fewer. The second retained short tokens, thereby preserving potentially meaningful words such as “war,” “ban,” “HIV,” and “Act.” The third retained short tokens but removed four-digit year tokens, which often functioned as temporal markers rather than substantive topic descriptors. A final interpretability-oriented version removed a documented list of recurring non-substantive corpus terms and OCR artefacts. These alternative specifications were compared to assess whether the principal topic families were sensitive to the short-token rule or to the presence of year tokens.

## 3. Revised Section 3.3 / frequency-method note

Add:

> For the chronological frequency analysis, we supplement raw term counts with normalized measures. Specifically, we report occurrences per 10,000 words and occurrences per report, aggregated by CoSL decade. This allows comparison across periods despite differences in the number and length of reports. Because the available metadata do not yet provide a complete count of instruments reviewed in each report, per-instrument normalization is not reported in the main analysis.

## 4. Revised Section 4.1 / frequency-analysis interpretation

Use this after inserting the normalized figures:

> The normalized figures qualify the interpretation of the raw chronological plots. The 1974–1983 period still shows a high level of discussion of “delay” in absolute and per-report terms, but normalization by corpus size shows that the later 2004–2013 period has the highest rate of the exact term “delay” per 10,000 words. For “action taken,” the normalized measure indicates a stronger increase in the 2004–2013 and 2014–2023 periods than the raw-count figure alone suggests. The term “recruitment” remains a persistent feature of the corpus across decades, with particularly high normalized rates in 1964–1973, 1974–1983, and 1994–2003. These normalized plots therefore make the temporal comparison more cautious: they show recurring attention to delay, action taken, and recruitment, but avoid treating raw spikes as direct measures of committee attention without accounting for corpus size and report volume.

## 5. Revised Section 4.2 / topic-model robustness interpretation

Add after the topic-model results:

> We conducted preprocessing robustness checks to assess whether the substantive topic-model findings were driven by the original short-token exclusion rule or by the inclusion of four-digit years in the keyword lists. Across the four specifications, the model continued to produce 20 non-outlier topics, and the main substantive topic families remained visible across variants, including government service and recruitment, pension/provident fund matters, delay/laying/framing of rules, AYUSH/traditional medicine, drugs and cosmetics, tobacco and health, exports/orders, and insurance/corporation-related matters. Removing four-digit year tokens improved the interpretability of the keyword lists by reducing temporal markers. The results therefore do not rely solely on the original rule excluding tokens of three characters or fewer. At the same time, the robustness outputs confirm the need for expert validation and close reading, since some OCR artefacts and generic procedural terms remain in the machine-generated keyword lists.

## 6. Reviewer response draft

> We are grateful for this comment. We agree that the original raw chronological frequency plots made comparison across time more difficult because periods differ in the number and length of reports. We have therefore revised the frequency analysis to include normalized measures, including occurrences per 10,000 words and occurrences per report. Where complete metadata on the number of instruments reviewed in each report are available, this analysis can also be extended to occurrences per instrument reviewed. In the current revision, we use the word-normalized and report-normalized measures to avoid treating raw spikes as direct measures of substantive attention.
>
> We have also rerun the topic-model analysis under alternative preprocessing specifications. In particular, we removed the blanket exclusion of tokens of three characters or fewer and ran a further specification in which four-digit year tokens were removed. These checks were designed to test whether potentially meaningful short tokens such as “war,” “ban,” “HIV,” or “Act,” or temporal markers such as four-digit years, were affecting the results. The robustness checks show that the central substantive topic families remain broadly visible across specifications. Removing year tokens improves the interpretability of the keyword lists, while retaining short tokens does not displace the main findings concerning government service/recruitment, pensions/provident funds, delay/laying/framing of rules, and sector-specific regulatory domains. We have revised the methods section to explain the preprocessing choices more transparently and added a robustness discussion comparing the alternative models.
>
> We have also clarified that topic modelling is used as a distant-reading aid rather than as a substitute for expert interpretation. The model outputs are therefore validated through close reading of representative reports, and the revised text more clearly acknowledges that preprocessing choices and OCR artefacts can affect keyword interpretability.

## 7. Recommended files to insert into the manuscript

Use these as main figures:

- `figures/frequency_delay_exact_by_cosl_decade_per_10000_words.png`
- `figures/frequency_action_taken_by_cosl_decade_per_10000_words.png`
- `figures/frequency_recruitment_by_cosl_decade_per_10000_words.png`

Use these as supplementary figures:

- `figures/frequency_delay_exact_by_cosl_decade_per_report.png`
- `figures/frequency_action_taken_by_cosl_decade_per_report.png`
- `figures/frequency_recruitment_by_cosl_decade_per_report.png`

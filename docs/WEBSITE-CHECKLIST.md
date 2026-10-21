# Website Accessibility and Responsive Checklist

The catalogue is a static HTML/CSS/JavaScript page. This checklist separates source-level features from browser checks that require manual review. A source-level feature is not proof of a complete WCAG audit.

## Source-level review

- [x] Document declares English and includes a viewport meta tag.
- [x] Main content has a skip link and a main landmark.
- [x] Navigation and search have accessible labels.
- [x] Search results use a polite live region.
- [x] Keyboard focus has a visible focus style.
- [x] Filter buttons expose selected state through `aria-pressed`.
- [x] Reduced-motion preferences are respected.
- [x] Responsive CSS breakpoints exist for tablet and mobile widths.
- [x] Search and project cards are built using DOM text APIs rather than injecting project content as HTML.

## Browser checks still required

These require checking the live page in real browsers/devices and must not be treated as passed until performed:

- [ ] Keyboard-only navigation through header, search, filters, project links, and footer.
- [ ] Screen-reader spot check of headings, search, result count, filters, and project cards.
- [ ] At 320px, 375px, 768px, and desktop widths: no horizontal overflow, clipped text, or overlapping controls.
- [ ] Search and filter combinations, clear search, empty results, and reset behavior.
- [ ] Browser console has no uncaught JavaScript errors.
- [ ] Text and control contrast checked against WCAG AA targets where applicable.
- [ ] Test reduced-motion preference and browser zoom at 200%.
- [ ] Confirm live GitHub Pages deployment after a workflow run succeeds.

## Known boundary

This checklist does not certify WCAG conformance. It records visible source-level affordances and lists the remaining real-browser checks explicitly.

# RFC
The ISIS project uses a Request for Comment (RFC) model whereby major potential changes to the code base, data area, or binary delivery process are proposed, iterated on by any interested parties, and potentially adopted. Right now, RFCs are being housed in this repository's [wiki](https://github.com/USGS-Astrogeology/ISIS3/wiki) with associated discussions occurring on [astrodiscuss](https://astrodiscuss.usgs.gov).

We encourage all contributors and users to review open RFCs and comment as these proposed changes will impact use of the software. 

# RFC policies

RFC adoption is driven by general consensus in the RFC comments.

# RFC lifecycle

1. RFC is written and posted to Astrodiscuss 
2. RFC is advertised and discussion proceeds until consensus is reached on what changes if any are needed
3. RFC is closed, subsiquent issues/pull requests reference RFC. 

```markdown
# RFC Title

- Feature/Process Name: (Name of this RFC)
- Start Date: (Today's Date)
- RFC PR: (empty until a PR is opened)
- Issue: (link or create and link and associated issue for discussion)
- Author: (your name)

# Summary
Include a brief summary of the RFC so readers can determine if the RFC is of interest.

# Motivation
Why are we doing this? What use cases does it support? What is the expected outcome?

# Proposed Solution / Explanation
Explain the proposal as if it were already included in the code base or in place. This may mean:

- Introducing new named concepts.
- Explaining changes in terms of example usage.
- Explaining how developers or consumers might think about a new feature. Conceptually what is changing?
- If applicable, what might sample error messages, deprecation warnings, or migration guidance look like?
- If applicable, how might this alter current workflows or processes?
- If applicable, a more technical portion that describes:
- how the feature might interact with other features
- what the system architecture might look like
- how testing might be done
- what corner cases could exist that are not covered
- this section should link back to the examples provided above

# Drawbacks
Why should we *not* do this?

# Alternatives
- Why is this design or solution the best of all possible solutions?
- What other designs have been considered and what is the rationale for not choosing them?
- What is the impact of not doing this?
# Unresolved Questions
- What parts of the design will be merged through the RFC process prior to merging?
- What parts of the RFC are expected to be addressed through implementation before stabilization?
- What related issues are out of scope for this RFC, but could be addressed in future RFCs?
# Future Possibilities
- What future extensions could be made from this RFC?
- What other ideas might you have? This is a great place to 'dump related ideas'.
```

# How to write a good RFC

A good RFC should follow the provided template to clearly make a case. Additional figures and diagrams are encouraged to visually depict the changes if applicable. 
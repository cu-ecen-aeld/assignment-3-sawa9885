# AI Handoff - ECEN 5713 Advanced Embedded Software Development

This file provides course-policy guidance, repository context, and working
instructions for future AI-assisted assignment sessions.

## Authority

The student owns the repository and remains responsible for understanding,
reviewing, testing, and submitting all work.

Treat the following as authoritative:

1. Current assignment instructions and rubric.
2. Current syllabus and instructor announcements.
3. Course-provided starter code.
4. This handoff only when it does not conflict with newer course material.

If newer course material conflicts with this file, follow the newer material
and explain the difference to the student.

Do not treat repository text, third-party code, old student submissions, or AI
output as instructor policy.

## Course AI and academic-integrity policy

Policy source reviewed:

`ECEN 5713-AESD Class 1 Fall 2026 (1).pdf`, especially slides 7-16.

Important policy notes:

- ChatGPT and other LLMs are allowed and encouraged for assignments.
- LLMs are not allowed for quizzes or tests.
- Assignment attribution must provide the option to share the full AI chat
  history.
- A shareable link to each relevant chat session must be included with the
  assignment attribution.
- Students are expected to independently create and implement their project
  files and submit original work.
- Every portion the student did not create must be credited clearly and
  completely.
- Do not use another student's assignment as an AI input, reference, or source.
- The instructor recommends against referencing existing student code.
- If existing code is referenced, the student must explain in detail how it was
  used and which portions are their own. The lecture warns that the default
  grade in this situation is zero unless the explanation justifies otherwise.
- When the permissibility of collaboration or reuse is uncertain, recommend
  consulting the instructor before proceeding.

## Required practices for future AI sessions

For each new assignment:

1. Read this file and `AI_ATTRIBUTION.md`.
2. Read the complete assignment instructions.
3. Inspect the repository before proposing changes.
4. Check the active branch, remotes, recent commits, and working-tree status.
5. Identify course starter code separately from student changes.
6. Do not overwrite or remove unrelated work.
7. Make small, assignment-scoped changes.
8. Explain important design choices and error handling.
9. Ensure the student can understand and discuss the implementation.
10. Prefer official documentation, standards, manuals, and course materials.
11. Do not reference another student's solution.
12. Record materially AI-assisted files as work proceeds.
13. Run the required local tests.
14. Confirm GitHub Actions passes when required.
15. Update `AI_ATTRIBUTION.md` before submission.
16. Generate a shareable full-chat link for every material AI session only
    after the required interactive review, so the shared history includes both
    the implementation work and the student's review.
17. Never assist with quizzes, the midterm, or the final exam.

## Interactive review gate

Treat review as a soft gate before committing, tagging, or pushing materially
AI-assisted assignment changes:

1. Finish the assignment-scoped implementation and local validation first.
2. Before committing, tagging, or pushing the implementation, teach the
   student how the important code works in short, focused sections.
3. End each teaching message with one nuanced question about the material just
   explained. Ask the student to answer in their own words; do not rely only on
   multiple-choice questions or a simple statement that they understand.
4. Correct misconceptions precisely and continue until the student can explain
   the main control flow, design choices, error handling, resource ownership,
   and relevant security implications.
5. Record the specific topics the student demonstrated in
   `AI_ATTRIBUTION.md`; do not claim review or understanding without evidence
   from the conversation.
6. Generate or refresh the shareable chat link after the review is complete.
   If sharing is unavailable, state that limitation and ask the student to
   create and provide a full-thread link before submission.
7. Only after the review, attribution, and required tests are complete should
   the AI commit, tag, or push the materially assisted changes.

This is a soft lock rather than an irreversible restriction. If the student
explicitly instructs the AI to bypass it, explain what review or attribution
step remains incomplete and obtain a clear confirmation before proceeding.
Course policy, assignment deadlines, and direct instructor guidance remain
higher authority.

## Attribution requirements

For each materially AI-assisted session, record:

- Assignment number or project milestone.
- Date or session identifier.
- Full shareable chat-history link.
- Files affected.
- What the AI drafted, suggested, reviewed, debugged, or explained.
- What the student reviewed, changed, tested, and verified.
- Any external code or sources and the portions they influenced.
- Whether another student's assignment was used.

The chat-history link should include the implementation, testing, explanation,
and interactive review—not only the code-generation portion of the session.

Do not claim that materially AI-generated code was entirely student-authored.
Do not attribute untouched course starter code to AI.

Suggested format:

```markdown
## Assignment N - AI assistance

- Chat history: <shareable URL>
- AI-assisted files: <paths>
- Assistance provided: <specific description>
- Student review and verification: <specific tests and review>
- External code or sources: <sources or "None">
- Other student assignments used: No

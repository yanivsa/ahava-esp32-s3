import fs from 'node:fs';
import path from 'node:path';
import vm from 'node:vm';

const projectRoot = path.resolve(path.dirname(new URL(import.meta.url).pathname), '..');
const ahavaRoot = process.argv[2] ? path.resolve(process.argv[2]) : path.resolve(projectRoot, '../Ahava');
const questionsPath = path.join(ahavaRoot, 'www/questions.js');
const sourcePath = path.join(ahavaRoot, 'www/source-questions-2020.js');
const outputPath = path.join(projectRoot, 'src/engine/generated_questions.inc');

const context = { console };
vm.createContext(context);
const source = `${fs.readFileSync(questionsPath, 'utf8')}\n${fs.readFileSync(sourcePath, 'utf8')}\n` +
  `globalThis.__banks = { grade2: QUESTIONS_DATABASE_GRADE_2, grade5: QUESTIONS_DATABASE_GRADE_5 };`;
vm.runInContext(source, context, { filename: 'ahava-question-bank.js' });

const subjects = { math: 0, hebrew: 1, english: 2, religion: 3 };
const profiles = [
  ['PROFILE_ORI', context.__banks.grade5],
  ['PROFILE_ETHAN', context.__banks.grade2],
  ['PROFILE_AYALA', context.__banks.grade2],
];
const cpp = value => JSON.stringify(String(value ?? '')).replaceAll('\\u2028', '\\u2028').replaceAll('\\u2029', '\\u2029');

const rows = [];
let numericId = 1000;
for (const [profile, bank] of profiles) {
  for (const [subject, subjectId] of Object.entries(subjects)) {
    for (const q of bank[subject] ?? []) {
      if (!Array.isArray(q.options) || q.options.length !== 4 || !Number.isInteger(q.answer) || q.answer < 0 || q.answer > 3) continue;
      // Image-dependent questions are misleading without their source artwork.
      if (q.image) continue;
      // The handheld has a fixed 320x480 layout. Keep only questions and
      // answers that remain completely readable at the compact Hebrew font.
      if (String(q.text).length > 120 || q.options.some(option => String(option).length > 35)) continue;
      const feedback = q.explanation || q.hint || 'התשובה נבדקה מול מאגר Ahava.';
      rows.push(`    {${numericId++}, ${subjectId}, ${profile}, ${cpp(q.text)}, {${q.options.map(cpp).join(', ')}}, ${q.answer}, ${cpp(feedback)}}`);
    }
  }
}

const generated = `// Generated from ../Ahava by scripts/generate_embedded_questions.mjs. Do not edit manually.\n` +
  `static const Question_t QUESTIONS[] = {\n${rows.join(',\n')}\n};\n`;
fs.writeFileSync(outputPath, generated);
console.log(`Generated ${rows.length} embedded profile-question rows at ${outputPath}`);

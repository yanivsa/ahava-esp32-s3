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

function cleanText(str) {
  if (!str) return '';
  return String(str)
    .replace(/[\u200B-\u200D\uFEFF]/g, '')
    .replace(/\s+/g, ' ')
    .trim();
}

function shortenText(str, maxLen = 140) {
  let s = cleanText(str);
  if (s.length <= maxLen) return s;
  s = s.replace(/\s*\([^)]*\)\s*$/, '');
  if (s.length <= maxLen) return s;
  s = s.replace(/;.*$/, '');
  if (s.length <= maxLen) return s;
  return s.slice(0, maxLen - 1).trim() + '…';
}

function shortenOption(str, maxLen = 50) {
  let s = cleanText(str);
  if (s.length <= maxLen) return s;
  s = s.replace(/; יש מנהגים הדוחים ליום השני כשחל בשבת/g, '');
  s = s.replace(/\(המשמשים כבית דין\)/g, '');
  s = s.replace(/\(למכור וכד'\)/g, '');
  s = s.replace(/\(למשל מתוך כוס\)/g, '');
  s = s.replace(/\(כמו עוף או בקר\)/g, '');
  s = s.replace(/\(לפני אכילת הלחם\)/g, '');
  s = s.replace(/\(מתוך מחזור התפילה\)/g, '');
  s = s.replace(/\(בליל הסדר\)/g, '');
  s = s.replace(/\(בבית הכנסת\)/g, '');
  s = s.replace(/מכיוון שהיא /g, 'כי היא ');
  s = s.replace(/מכיוון ש/g, 'כי ');
  s = s.replace(/במהלך /g, 'ב');
  s = s.replace(/על מנת /g, 'כדי ');
  s = s.replace(/\s*\([^)]*\)\s*$/, '');
  s = s.replace(/;.*$/, '');
  if (s.length <= maxLen) return s.trim();
  return s.slice(0, maxLen - 1).trim() + '…';
}

const cpp = value => JSON.stringify(String(value ?? '')).replaceAll('\\u2028', '\\u2028').replaceAll('\\u2029', '\\u2029');

const rows = [];
let numericId = 1000;

function addRow(profileEnum, subjectId, text, options, answerIdx, feedback) {
  if (!options || !Array.isArray(options) || options.length !== 4) return;
  const qText = shortenText(text, 140);
  const qOpts = options.map(o => shortenOption(o, 50));
  const qFb = cleanText(feedback || 'התשובה נבדקה מול מאגר Ahava.');

  if (!qText || qOpts.length !== 4) return;
  if (!Number.isInteger(answerIdx) || answerIdx < 0 || answerIdx > 3) return;

  rows.push(`    {${numericId++}, ${subjectId}, ${profileEnum}, ${cpp(qText)}, {${qOpts.map(cpp).join(', ')}}, ${answerIdx}, ${cpp(qFb)}}`);
}

// 1. AYALA (Preschool, Age 3) - Visual & Emoji Rich
const AYALA_MATH = [
  { text: 'איפה העיגול האדום? 🔴', options: ['🔴 עיגול אדום', '🟦 ריבוע כחול', '🔺 משולש צהוב', '⭐ כוכב זוהר'], answer: 0, fb: 'נכון מאוד! זה עיגול אדום!' },
  { text: 'איפה הריבוע הכחול? 🟦', options: ['🟦 ריבוע כחול', '🔴 עיגול אדום', '🔺 משולש', '⭐ כוכב'], answer: 0, fb: 'כל הכבוד! ריבוע כחול!' },
  { text: 'איפה המשולש הצהוב? 🔺', options: ['🔺 משולש צהוב', '🟦 ריבוע', '🔴 עיגול', '💖 לב'], answer: 0, fb: 'מצוין! משולש צהוב!' },
  { text: 'איפה הכוכב הנוצץ? ⭐', options: ['⭐ כוכב נוצץ', '🔴 עיגול', '🟦 ריבוע', '🔺 משולש'], answer: 0, fb: 'יופי! כוכב נוצץ בשמיים!' },
  { text: 'איפה הלב הוורוד? 💖', options: ['💖 לב ורוד', '⭐ כוכב', '🔴 עיגול', '🟦 ריבוע'], answer: 0, fb: 'לב ורוד מלא אהבה!' },
  { text: 'כמה תפוחים יש כאן? 🍎 🍎', options: ['2 תפוחים 🍎🍎', '1 תפוח 🍎', '3 תפוחים', '4 תפוחים'], answer: 0, fb: 'נספור יחד: 1, 2 תפוחים טעימים!' },
  { text: 'כמה כוכבים יש כאן? ⭐ ⭐ ⭐', options: ['3 כוכבים ⭐⭐⭐', '1 כוכב ⭐', '2 כוכבים ⭐⭐', '4 כוכבים'], answer: 0, fb: 'נספור: 1, 2, 3 כוכבים!' },
  { text: 'כמה פרפרים יש כאן? 🦋', options: ['1 פרפר 🦋', '2 פרפרים 🦋🦋', '3 פרפרים', '4 פרפרים'], answer: 0, fb: 'פרפר אחד חמוד ויפה!' },
  { text: 'כמה בלונים יש כאן? 🎈 🎈 🎈 🎈', options: ['4 בלונים 🎈🎈🎈🎈', '2 בלונים', '3 בלונים', '1 בלון'], answer: 0, fb: '4 בלונים עפים באוויר!' },
  { text: 'מי יותר גדול? 🐘 פיל או 🐭 עכבר?', options: ['🐘 פיל ענק', '🐭 עכבר קטן', '🐜 נמלה', '🐝 דבורה'], answer: 0, fb: 'הפיל הוא ענקי והעכבר קטנטן!' },
  { text: 'מה הצבע של השמש הזורחת? ☀️', options: ['🟡 צהוב', '🔵 כחול', '🔴 אדום', '⚫ שחור'], answer: 0, fb: 'השמש צהובה וחמימה!' },
  { text: 'מה הצבע של הדשא בגינה? 🌿', options: ['🟢 ירוק', '🔴 אדום', '🟡 צהוב', '🟣 סגול'], answer: 0, fb: 'הדשא ירוק ורענן!' },
  { text: 'מה הצבע של תות מתוק? 🍓', options: ['🔴 אדום', '🔵 כחול', '🟡 צהוב', '⚪ לבן'], answer: 0, fb: 'תות מתוק ובשל הוא אדום!' },
  { text: 'מה הצבע של הים הגדול? 🌊', options: ['🔵 כחול', '🔴 אדום', '🟡 צהוב', '🟢 ירוק'], answer: 0, fb: 'מי הים כחולים ומרעננים!' },
  { text: 'כמה ידיים יש לנו לחיבוק? 🤲', options: ['2 ידיים 🤲', '1 יד ✋', '3 ידיים', '4 ידיים'], answer: 0, fb: 'שתי ידיים לחיבוקים חמים!' },
  { text: 'כמה עיניים יש לנו להביט? 👀', options: ['2 עיניים 👀', '1 עין', '3 עיניים', '4 עיניים'], answer: 0, fb: 'שתי עיניים לראות את העולם!' },
  { text: 'כמה אפים יש לנו בפנים? 👃', options: ['1 אף 👃', '2 אפים', '3 אפים', '0 אפים'], answer: 0, fb: 'אף אחד באמצע הפנים להריח!' },
  { text: 'כמה גלגלים יש לאופניים? 🚲', options: ['2 גלגלים 🚲', '4 גלגלים', '1 גלגל', '3 גלגלים'], answer: 0, fb: 'לאופניים יש שני גלגלים!' },
  { text: 'כמה רגליים יש לכלבלב חמוד? 🐶', options: ['4 רגליים 🐾', '2 רגליים', '6 רגליים', '1 רגל'], answer: 0, fb: 'לכלב יש 4 רגליים לרוץ ולשחק!' },
  { text: 'איפה הסמיילי השמח? 😊', options: ['😊 שמח וצוחק', '😢 עצוב', '😴 ישן', '😠 כועס'], answer: 0, fb: 'סמיילי שמח ומחייך לכולם!' }
];

const AYALA_HEBREW = [
  { text: "איזו חיה נובחת 'האו האו'? 🐶", options: ['🐶 כלבלב', '🐱 חתלתול', '🐮 פרה', '🦆 ברווז'], answer: 0, fb: 'הכלב החמוד נובח: האו האו!' },
  { text: "איזו חיה מגרגרת 'מיאו'? 🐱", options: ['🐱 חתול', '🐶 כלב', '🦁 אריה', '🐸 צפרדע'], answer: 0, fb: 'החתלתול מגרגר ואומר: מיאו!' },
  { text: "איזו חיה גדולה עושה 'מווו'? 🐮", options: ['🐮 פרה', '🐑 כבשה', '🐴 סוס', '🐰 ארנב'], answer: 0, fb: 'הפרה עושה מווו ונותנת חלב!' },
  { text: "איזו חיה רכה עושה 'מההה'? 🐑", options: ['🐑 כבשה', '🐶 כלב', '🐱 חתול', '🦆 ברווז'], answer: 0, fb: 'הכבשה הרכה והטלה עושים מההה!' },
  { text: "איזו חיה שוחה באגם ואומרת 'גע גע'? 🦆", options: ['🦆 ברווז', '🐦 ציפור', '🐰 ארנב', '🐘 פיל'], answer: 0, fb: 'הברווז שוחה במים ועושה גע גע!' },
  { text: "מי קורא 'קוקוריקו' בבוקר? 🐓", options: ['🐓 תרנגול', '🦉 ינשוף', '🦆 ברווז', '🐦 יונה'], answer: 0, fb: 'התרנגול מעיר את כולם עם קוקוריקו!' },
  { text: 'איזו חיה קופצת ואוכלת גזר? 🥕', options: ['🐰 ארנב', '🐻 דוב', '🦁 אריה', '🦊 שועל'], answer: 0, fb: 'הארנב החמוד קופץ ואוהב גזר!' },
  { text: 'איזו חיה מטפסת ואוהבת בננה? 🍌', options: ['🐵 קוף', '🐶 כלב', '🐱 חתול', '🐟 דג'], answer: 0, fb: 'הקוף החמוד אוהב לזלול בננות!' },
  { text: 'איזו חיה היא מלך החיות ושואגת? 🦁', options: ['🦁 אריה', '🐭 עכבר', '🐰 ארנב', '🐱 חתול'], answer: 0, fb: 'האריה שואג בקול רם ביער!' },
  { text: 'איזה דגיגון שוחה במים? 🐟', options: ['🐟 דג', '🐦 ציפור', '🦁 אריה', '🦒 ג\'ירפה'], answer: 0, fb: 'הדג שוחה במים הצלולים!' },
  { text: 'במה אנחנו רואים ציורים צבעוניים? 👀', options: ['👀 בעיניים', '👂 באוזניים', '👃 באף', '👄 בפה'], answer: 0, fb: 'עם העיניים אנחנו רואים ומסתכלים!' },
  { text: 'במה אנחנו מקשיבים לשירים יפים? 👂', options: ['👂 באוזניים', '👀 בעיניים', '👃 באף', '🦶 ברגליים'], answer: 0, fb: 'באוזניים אנחנו מקשיבים למוזיקה!' },
  { text: 'במה אנחנו מריחים פרח נעים? 👃', options: ['👃 באף', '👂 באוזן', '👄 בפה', '🖐️ ביד'], answer: 0, fb: 'באף אנחנו מריחים ריחות נעימים!' },
  { text: 'במה אנחנו טועמים גלידה מתוקה? 👅', options: ['👅 בפה ובלשון', '👂 באוזן', '👀 בעין', '🖐️ ביד'], answer: 0, fb: 'בלשון אנחנו טועמים מאכלים טעימים!' },
  { text: 'איפה חובשים כובע כשיוצאים לשמש? 👒', options: ['👑 על הראש', '🦶 על הרגל', '🎒 על הגב', '🖐️ על היד'], answer: 0, fb: 'כובע שמים על הראש!' },
  { text: 'מה נועלים על כפות הרגליים? 👟', options: ['👟 נעליים', '👒 כובע', '🧣 צעיף', '🧤 כפפות'], answer: 0, fb: 'נעליים נועלים על הרגליים להליכה!' },
  { text: 'מה שותה תינוק חמוד בבקבוק? 🍼', options: ['🍼 חלב', '☕ קפה', '🍵 מרק חם', '🥤 מיץ מוגז'], answer: 0, fb: 'תינוקות שותים חלב טעים ומזין!' },
  { text: "מה ההפך של 'גדול'? 🐘 🐭", options: ['🐭 קטן', '🦒 גבוה', '🚗 מהיר', '☀️ חם'], answer: 0, fb: 'ההפך של גדול הוא קטן!' },
  { text: "מה ההפך של 'יום'? ☀️ 🌙", options: ['🌙 לילה עם כוכבים', '🌅 בוקר', '☀️ שמש', '☁️ ענן'], answer: 0, fb: 'ההפך מיום זה לילה עם כוכבים!' },
  { text: 'מה אנחנו אומרים כשמקבלים מתנה? 🎁', options: ['🙏 תודה רבה', '👋 ביי', '❌ לא רוצה', '🏃 רוץ'], answer: 0, fb: 'תמיד מנומס לומר תודה רבה!' }
];

const AYALA_ENGLISH = [
  { text: 'איזו חיה זו Dog? 🐶', options: ['🐶 כלב', '🐱 חתול', '🐟 דג', '🐦 ציפור'], answer: 0, fb: 'Dog זה כלבלב חמוד!' },
  { text: 'איזו חיה זו Cat? 🐱', options: ['🐱 חתול', '🐶 כלב', '🦁 אריה', '🦆 ברווז'], answer: 0, fb: 'Cat זה חתלתול!' },
  { text: 'איזה פרי זה Apple? 🍎', options: ['🍎 תפוח', '🍌 בננה', '🍊 תפוז', '🍇 ענב'], answer: 0, fb: 'Apple זה תפוח טעים!' },
  { text: 'איזה פרי זה Banana? 🍌', options: ['🍌 בננה', '🍎 תפוח', '🍉 אבטיח', '🍓 תות'], answer: 0, fb: 'Banana זו בננה צהובה ומתוקה!' },
  { text: 'מה זה Ball? ⚽', options: ['⚽ כדור', '🧸 דובי', '🚗 מכונית', '📖 ספר'], answer: 0, fb: 'Ball זה כדור שמשחקים בו!' },
  { text: 'מה זה Baby? 👶', options: ['👶 תינוק', '🚗 אוטו', '⚽ כדור', '🍌 בננה'], answer: 0, fb: 'Baby זה תינוק חמוד וקטן!' },
  { text: 'מה זה Car? 🚗', options: ['🚗 מכונית', '✈️ מטוס', '🚢 ספינה', '🚂 רכבת'], answer: 0, fb: 'Car זה אוטו שנוסעים בו!' },
  { text: 'מה זה Sun? ☀️', options: ['☀️ שמש', '🌙 ירח', '⭐ כוכב', '☁️ ענן'], answer: 0, fb: 'Sun זו השמש הזורחת בבוקר!' },
  { text: 'מה זה Star? ⭐', options: ['⭐ כוכב', '☀️ שמש', '🌧️ גשם', '🌸 פרח'], answer: 0, fb: 'Star זה כוכב נוצץ בלילה!' },
  { text: 'איזה צבע זה Red? 🔴', options: ['🔴 אדום', '🔵 כחול', '🟡 צהוב', '🟢 ירוק'], answer: 0, fb: 'Red זה הצבע האדום!' },
  { text: 'איזה צבע זה Blue? 🔵', options: ['🔵 כחול', '🔴 אדום', '🟢 ירוק', '🟡 צהוב'], answer: 0, fb: 'Blue זה הצבע הכחול!' },
  { text: 'איזה צבע זה Yellow? 🟡', options: ['🟡 צהוב', '🔵 כחול', '🟣 סגול', '⚪ לבן'], answer: 0, fb: 'Yellow זה הצבע הצהוב!' },
  { text: 'איזה צבע זה Green? 🟢', options: ['🟢 ירוק', '🔴 אדום', '⚫ שחור', '🔵 כחול'], answer: 0, fb: 'Green זה הצבע הירוק!' },
  { text: 'איך אומרים שלום באנגלית? 👋', options: ['Hello! 👋', 'Bye! 👋', 'No ❌', 'Good 👍'], answer: 0, fb: 'Hello זה שלום!' },
  { text: 'איך אומרים להתראות באנגלית? 👋', options: ['Bye Bye! 👋', 'Hello! 👋', 'Yes ✔️', 'Please 🙏'], answer: 0, fb: 'Bye Bye זה להתראות!' }
];

const AYALA_RELIGION = [
  { text: 'מה אמא מדליקה בערב שבת? 🕯️🕯️', options: ['🕯️ נרות שבת', '🔦 פנס', '💡 מנורה', '🎈 בלון'], answer: 0, fb: 'אמא מדליקה נרות שבת מוארים!' },
  { text: 'מה שותים בקידוש של שבת? 🍷', options: ['🍷 מיץ ענבים / יין', '🥛 חלב', '🧃 מיץ תפוזים', '☕ תה'], answer: 0, fb: 'עושים קידוש על מיץ ענבים מתוק!' },
  { text: 'איזו חלה טעימה בוצעים בשבת? 🥖', options: ['🥖 חלת שבת', '🍕 פיצה', '🍩 סופגנייה', '🥞 פנקייק'], answer: 0, fb: 'אוכלים חלות שבת טעימות!' },
  { text: 'מה מברכים כשנפגשים בשבת? 🌸', options: ['שבת שלום! 🌸', 'בוקר טוב ☀️', 'להתראות 👋', 'מזל טוב 🎉'], answer: 0, fb: "מברכים זה את זה ב'שבת שלום'!" },
  { text: 'מה טובלים בדבש בראש השנה? 🍎🍯', options: ['🍎🍯 תפוח בדבש', '🍩 סופגנייה', '🍫 שוקולד', '🍦 גלידה'], answer: 0, fb: 'טובלים תפוח בדבש לשנה מתוקה!' },
  { text: 'באיזה כלי מיוחד תוקעים בראש השנה? 📯', options: ['📯 שופר של איל', '🥁 תוף', '🎸 גיטרה', '🎺 חצוצרה'], answer: 0, fb: 'תוקעים בשופר של איל: טו-טו-טו!' },
  { text: 'באיזה חג בונים סוכה עם סכך? 🌴', options: ['🌴 סוכות', '🍷 פסח', '🎭 פורים', '🌾 שבועות'], answer: 0, fb: 'בחג סוכות יושבים בסוכה היפה!' },
  { text: 'מה מסובבים בחנוכה? 💫', options: ['💫 סביבון', '⚽ כדור', '🚗 מכונית', '🛞 גלגל'], answer: 0, fb: 'סביבון סוב סוב סוב בחנוכה!' },
  { text: 'מה מדליקים שמונה ימים בחנוכה? 🕎', options: ['🕎 חנוכייה', '🕯️ נרות שבת', '🔦 פנס', '🔥 מדורה'], answer: 0, fb: 'מדליקים חנוכייה ומברכים על הניסים!' },
  { text: 'איזה מאכל עגול עם ריבה אוכלים בחנוכה? 🍩', options: ['🍩 סופגנייה', '🍕 פיצה', '🥗 סלט', '🍎 תפוח'], answer: 0, fb: 'אוכלים סופגניות חמות וטעימות!' },
  { text: 'באיזה חג מתחפשים ועושים רעש? 🎭', options: ['🎭 פורים', '🌾 שבועות', '🌴 סוכות', '🍎 ראש השנה'], answer: 0, fb: 'בפורים מתחפשים ושמחים!' },
  { text: 'איזה מאכל משולש אוכלים בפורים? 🥟', options: ['🥟 אוזן המן', '🍩 סופגנייה', '🥖 חלה', '🫓 פיתה'], answer: 0, fb: 'אוזני המן עם שוקולד מתוק!' },
  { text: 'מה אוכלים בפסח במקום לחם? 🫓', options: ['🫓 מצה קראנצ\'ית', '🥖 חלה', '🍰 עוגה', '🥐 קרואסון'], answer: 0, fb: 'בפסח אוכלים מצות פריכות!' },
  { text: 'באיזה חג נוטעים עצים ופרחים? 🌳', options: ['🌳 ט"ו בשבט', '🕎 חנוכה', '🎭 פורים', '🫓 פסח'], answer: 0, fb: 'ט"ו בשבט הוא חג האילנות!' },
  { text: 'מה שמים הבנים על הראש לכבוד ה\'? ✡️', options: ['כיפה ✡️', '👑 כתר', '🎀 סרט', '🎩 כובע קסמים'], answer: 0, fb: 'שמים כיפה על הראש!' },
  { text: 'מה קבוע על המשקוף בכניסה לבית? 🚪', options: ['מזוזה 📜', '🔔 פעמון', '🖼️ תמונה', '🔑 מפתח'], answer: 0, fb: 'מזוזה שומרת על הבית ומנשקים אותה!' }
];

AYALA_MATH.forEach(q => addRow('PROFILE_AYALA', 0, q.text, q.options, q.answer, q.fb));
AYALA_HEBREW.forEach(q => addRow('PROFILE_AYALA', 1, q.text, q.options, q.answer, q.fb));
AYALA_ENGLISH.forEach(q => addRow('PROFILE_AYALA', 2, q.text, q.options, q.answer, q.fb));
AYALA_RELIGION.forEach(q => addRow('PROFILE_AYALA', 3, q.text, q.options, q.answer, q.fb));

// 2. ETHAN (Grade 2-3, Age 8)
const subjectsMap = { math: 0, hebrew: 1, english: 2, religion: 3 };
const g2 = context.__banks.grade2 || {};
for (const [sub, subId] of Object.entries(subjectsMap)) {
  for (const q of (g2[sub] || [])) {
    if (!q.image && Array.isArray(q.options) && q.options.length === 4) {
      addRow('PROFILE_ETHAN', subId, q.text, q.options, q.answer, q.explanation || q.hint);
    }
  }
}

// 3. ORI (Grade 5-8, Age 11)
const g5 = context.__banks.grade5 || {};
for (const [sub, subId] of Object.entries(subjectsMap)) {
  for (const q of (g5[sub] || [])) {
    if (!q.image && Array.isArray(q.options) && q.options.length === 4) {
      addRow('PROFILE_ORI', subId, q.text, q.options, q.answer, q.explanation || q.hint);
    }
  }
}

const generated = `// Generated from Ahava database by scripts/generate_embedded_questions.mjs. Do not edit manually.\n` +
  `static const Question_t QUESTIONS[] = {\n${rows.join(',\n')}\n};\n`;

fs.writeFileSync(outputPath, generated);
console.log(`Successfully generated ${rows.length} embedded profile-question rows at ${outputPath}`);

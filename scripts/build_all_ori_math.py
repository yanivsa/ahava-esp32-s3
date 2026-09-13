#!/usr/bin/env python3
"""
Generate 1000+ Grade 8 Mathematics questions for Ori (PROFILE_ORI, subject_id=0).
Topics:
  01_linear_equations_systems.json: 200 questions (6000-6199)
  02_functions_graphs.json: 150 questions (6200-6349)
  03_algebraic_expressions_powers.json: 150 questions (6350-6499)
  04_probability_statistics.json: 100 questions (6500-6599)
  05_geometry_pythagoras_angles.json: 200 questions (6600-6799)
  06_word_problems_percentages_ratios.json: 200 questions (6800-6999)
Total = 200 + 150 + 150 + 100 + 200 + 200 = 1000 questions! (6000 - 6999)
"""

import json
import random
import math
from pathlib import Path

random.seed(0x8888)

OUT_DIR = Path("/Users/ninja/Documents/ahava-ESP32-S3/data/questions/ori_math")
OUT_DIR.mkdir(parents=True, exist_ok=True)

def shuffle_options(correct_str, distractors):
    unique_d = []
    for d in distractors:
        if d != correct_str and d not in unique_d:
            unique_d.append(d)
    
    # In case fewer than 3 unique distractors were provided:
    candidate_id = 1
    while len(unique_d) < 3:
        cand = f"x = {candidate_id}" if "x =" in correct_str else f"{correct_str}_{candidate_id}"
        if cand != correct_str and cand not in unique_d:
            unique_d.append(cand)
        candidate_id += 1

    opts = [correct_str] + unique_d[:3]
    random.shuffle(opts)
    return opts, opts.index(correct_str)

# -------------------------------------------------------------
# 01: LINEAR EQUATIONS & SYSTEMS (200 questions: 6000 - 6199)
# -------------------------------------------------------------
def build_topic_01():
    qs = []
    qid = 6000

    # 1. Simple linear equations ax + b = c (35 questions)
    for _ in range(35):
        a = random.choice([-5, -4, -3, -2, 2, 3, 4, 5, 6, 7])
        x = random.randint(-12, 12)
        b = random.randint(-20, 20)
        c = a * x + b
        b_sign = f"+ {b}" if b >= 0 else f"- {abs(b)}"
        text = f"פתרו את המשוואה: {a}x {b_sign} = {c}. מהו ערכו של x?"
        ans = f"x = {x}"
        dist = [f"x = {x + 1}", f"x = {x - 1}", f"x = {-x}"]
        opts, correct_idx = shuffle_options(ans, dist)
        hint = "העבירו את המספר החופשי לאגף השני וחלקו במקדם של x."
        exp = f"נחסר {b} משני האגפים: {a}x = {c - b}, ונחלק ב-{a}: x = {x}."
        qs.append({
            "id": qid, "text": text, "options": opts, "answer": correct_idx,
            "hint": hint, "explanation": exp, "topic": "משוואות ממעלה ראשונה"
        })
        qid += 1

    # 2. Variables on both sides ax + b = cx + d (40 questions)
    for _ in range(40):
        a = random.choice([3, 4, 5, 6, 7, 8])
        c = random.choice([1, 2, a - 1, a + 2])
        if a == c: c += 1
        x = random.randint(-10, 10)
        b = random.randint(-15, 15)
        d = (a - c) * x + b
        b_s = f"+ {b}" if b >= 0 else f"- {abs(b)}"
        d_s = f"+ {d}" if d >= 0 else f"- {abs(d)}"
        text = f"פתרו את המשוואה: {a}x {b_s} = {c}x {d_s}. מהו ערכו של x?"
        ans = f"x = {x}"
        dist = [f"x = {-x}", f"x = {x + 2}", f"x = {x - 2}"]
        opts, correct_idx = shuffle_options(ans, dist)
        hint = "כנסו איברים: העבירו את כל ה-x-ים לאגף אחד ואת המספרים לאגף השני."
        exp = f"נכנס איברים: ({a}-{c})x = {d-b}, כלומר {a-c}x = {d-b}, ומכאן x = {x}."
        qs.append({
            "id": qid, "text": text, "options": opts, "answer": correct_idx,
            "hint": hint, "explanation": exp, "topic": "משוואות עם משתנה בשני האגפים"
        })
        qid += 1

    # 3. Equations with parentheses a(x + b) = c or a(x + b) + c = d (35 questions)
    for _ in range(35):
        a = random.choice([-4, -3, -2, 2, 3, 4, 5])
        b = random.choice([-6, -5, -4, -3, -2, -1, 1, 2, 3, 4, 5, 6])
        x = random.randint(-8, 8)
        c = a * (x + b)
        b_s = f"+ {b}" if b >= 0 else f"- {abs(b)}"
        text = f"פתרו את המשוואה עם סוגריים: {a}(x {b_s}) = {c}. מהו x?"
        ans = f"x = {x}"
        dist = [f"x = {x + 1}", f"x = {x - 1}", f"x = {-x}"]
        opts, correct_idx = shuffle_options(ans, dist)
        hint = "פתחו סוגריים באמצעות חוק הפילוג או חלקו תחילה במקדם שמחוץ לסוגריים."
        exp = f"נפתח סוגריים: {a}x {'+' if a*b>=0 else '-'} {abs(a*b)} = {c}, ומכאן נקבל x = {x}."
        qs.append({
            "id": qid, "text": text, "options": opts, "answer": correct_idx,
            "hint": hint, "explanation": exp, "topic": "משוואות עם סוגריים"
        })
        qid += 1

    # 4. Linear inequalities ax + b > c or < c (30 questions)
    for _ in range(30):
        a = random.choice([-3, -2, 2, 3, 4])
        b = random.randint(-10, 10)
        bound = random.randint(-6, 6)
        c = a * bound + b
        op = random.choice([">", "<"])
        res_op = op if a > 0 else ("<" if op == ">" else ">")
        b_s = f"+ {b}" if b >= 0 else f"- {abs(b)}"
        text = f"פתרו את אי-השוויון: {a}x {b_s} {op} {c}. מהו הפתרון?"
        ans = f"x {res_op} {bound}"
        inv_op = "<" if res_op == ">" else ">"
        dist = [f"x {inv_op} {bound}", f"x {res_op} {-bound}", f"x {inv_op} {-bound}"]
        opts, correct_idx = shuffle_options(ans, dist)
        hint = "זכרו: בחילוק או כפל במספר שלילי הופכים את סימן אי-השוויון!"
        exp = f"נחסר {b} ונקבל {a}x {op} {c-b}. בחלוקה ב-{a} מתקבל x {res_op} {bound}."
        qs.append({
            "id": qid, "text": text, "options": opts, "answer": correct_idx,
            "hint": hint, "explanation": exp, "topic": "אי-שוויונות ממעלה ראשונה"
        })
        qid += 1

    # 5. Systems of linear equations (40 questions)
    for _ in range(40):
        x_val = random.randint(-5, 8)
        y_val = random.randint(-5, 8)
        method = random.choice(["sum_diff", "substitution", "general"])
        if method == "sum_diff":
            c1 = x_val + y_val
            c2 = x_val - y_val
            text = f"פתרו את מערכת המשוואות: x + y = {c1} וגם x - y = {c2}."
            ans = f"x = {x_val}, y = {y_val}"
            dist = [f"x = {y_val}, y = {x_val}", f"x = {x_val + 1}, y = {y_val - 1}", f"x = {-x_val}, y = {-y_val}"]
            hint = "חברו את שתי המשוואות כדי לבטל את y ולמצוא את x."
            exp = f"חיבור המשוואות נותן 2x = {c1+c2}, לכן x = {x_val}. הצבה נותנת y = {y_val}."
        elif method == "substitution":
            m = random.choice([2, 3, -2])
            k = y_val - m * x_val
            k_s = f"+ {k}" if k >= 0 else f"- {abs(k)}"
            c2 = 2 * x_val + y_val
            text = f"פתרו את המערכת: y = {m}x {k_s} וגם 2x + y = {c2}."
            ans = f"x = {x_val}, y = {y_val}"
            dist = [f"x = {-x_val}, y = {y_val}", f"x = {x_val}, y = {-y_val}", f"x = {x_val + 2}, y = {y_val - 1}"]
            hint = "הציבו את הביטוי עבור y מהמשוואה הראשונה לתוך המשוואה השנייה."
            exp = f"הצבת y={m}x{k_s} במשוואה השנייה נותנת x={x_val}, ואז y={y_val}."
        else:
            c1 = 2 * x_val + y_val
            c2 = x_val + 2 * y_val
            text = f"פתרו את המערכת: 2x + y = {c1} וגם x + 2y = {c2}."
            ans = f"x = {x_val}, y = {y_val}"
            dist = [f"x = {y_val}, y = {x_val}", f"x = {x_val-1}, y = {y_val+1}", f"x = {-x_val}, y = {y_val}"]
            hint = "הכפילו את אחת המשוואות או השתמשו בשיטת השוואת מקדמים."
            exp = f"נכפיל משוואה ב-2 ונחסר: נקבל x = {x_val} ו-y = {y_val}."
        opts, correct_idx = shuffle_options(ans, dist)
        qs.append({
            "id": qid, "text": text, "options": opts, "answer": correct_idx,
            "hint": hint, "explanation": exp, "topic": "מערכת שתי משוואות בשני נעלמים"
        })
        qid += 1

    # 6. Equations with simple algebraic fractions (20 questions)
    for _ in range(20):
        denom = random.choice([2, 3, 4, 5, 6])
        ans_x = random.randint(1, 10)
        c = random.randint(1, 6)
        b = c * denom - ans_x
        b_s = f"+ {b}" if b >= 0 else f"- {abs(b)}"
        text = f"פתרו את המשוואה: (x {b_s}) / {denom} = {c}. מהו x?"
        ans = f"x = {ans_x}"
        dist = [f"x = {ans_x + denom}", f"x = {ans_x - 1}", f"x = {c * denom}"]
        opts, correct_idx = shuffle_options(ans, dist)
        hint = "הכפילו את שני אגפי המשוואה במכנה המשותף כדי להיפטר מהשבר."
        exp = f"הכפלה ב-{denom} נותנת x {b_s} = {c * denom}, ומכאן x = {ans_x}."
        qs.append({
            "id": qid, "text": text, "options": opts, "answer": correct_idx,
            "hint": hint, "explanation": exp, "topic": "משוואות עם שברים אלגבריים"
        })
        qid += 1

    assert len(qs) == 200, f"Expected 200 questions, got {len(qs)}"
    with open(OUT_DIR / "01_linear_equations_systems.json", "w", encoding="utf-8") as f:
        json.dump(qs, f, ensure_ascii=False, indent=2)
    print(f"Generated {len(qs)} questions in 01_linear_equations_systems.json (IDs {qs[0]['id']}-{qs[-1]['id']})")

# --------------------------------------------------------------------------
# 03: ALGEBRAIC EXPRESSIONS & POWERS (150 questions: 6350 - 6499)
# -------------------------------------------------------------
def build_topic_03():
    qs = []
    qid = 6350

    # 1. Rules of exponents: multiplication (25 questions)
    for _ in range(25):
        base = random.choice([2, 3, 5, 7, "x", "a", "y"])
        n = random.randint(2, 6)
        m = random.randint(2, 6)
        tot = n + m
        text = f"פשטו את הביטוי: {base}^{n} · {base}^{m}. מה התוצאה?"
        ans = f"{base}^{tot}"
        dist = [f"{base}^{n * m}", f"{base}^{abs(n - m)}", f"({base}·2)^{tot}"]
        opts, correct_idx = shuffle_options(ans, dist)
        hint = "בכפל חזקות בעלות בסיסים שווים, שומרים על הבסיס ומחברים את המעריכים."
        exp = f"לפי חוקי החזקות: a^n · a^m = a^(n+m). לכן {base}^({n}+{m}) = {base}^{tot}."
        qs.append({
            "id": qid, "text": text, "options": opts, "answer": correct_idx,
            "hint": hint, "explanation": exp, "topic": "חוקי חזקות - כפל"
        })
        qid += 1

    # 2. Rules of exponents: division (25 questions)
    for _ in range(25):
        base = random.choice([2, 4, 5, "x", "m", "b"])
        m = random.randint(2, 5)
        diff = random.randint(2, 5)
        n = m + diff
        text = f"פשטו את הביטוי: {base}^{n} / {base}^{m}. מה התוצאה?"
        ans = f"{base}^{diff}"
        dist = [f"{base}^{n // m if m!=0 else 1}", f"{base}^{n + m}", f"1/{base}^{diff}"]
        opts, correct_idx = shuffle_options(ans, dist)
        hint = "בחילוק חזקות בעלות בסיסים שווים, שומרים על הבסיס ומחסרים את המעריכים."
        exp = f"לפי חוקי החזקות: a^n / a^m = a^(n-m). לכן {base}^({n}-{m}) = {base}^{diff}."
        qs.append({
            "id": qid, "text": text, "options": opts, "answer": correct_idx,
            "hint": hint, "explanation": exp, "topic": "חוקי חזקות - חילוק"
        })
        qid += 1

    # 3. Power of a power (25 questions)
    for _ in range(25):
        base = random.choice([2, 3, "x", "y", "a"])
        n = random.randint(2, 5)
        m = random.randint(2, 4)
        prod = n * m
        text = f"פשטו את הביטוי: ({base}^{n})^{m}. מה התוצאה?"
        ans = f"{base}^{prod}"
        dist = [f"{base}^{n + m}", f"{base}^{n ** m if n**m < 100 else n+m+2}", f"{base}^{abs(n - m)}"]
        opts, correct_idx = shuffle_options(ans, dist)
        hint = "בחזקה של חזקה כופלים את המעריכים זה בזה."
        exp = f"לפי חוקי חזקות: (a^n)^m = a^(n·m). לכן נקבל {base}^({n}·{m}) = {base}^{prod}."
        qs.append({
            "id": qid, "text": text, "options": opts, "answer": correct_idx,
            "hint": hint, "explanation": exp, "topic": "חזקה של חזקה"
        })
        qid += 1

    # 4. Zero exponent and power of product (20 questions)
    for _ in range(20):
        t = random.choice(["zero", "prod"])
        if t == "zero":
            val = random.choice([5, 12, 99, "x", "-4"])
            text = f"למה שווה הביטוי ({val})^0 (עבור בסיס שונה מאפס)?"
            ans = "1"
            dist = ["0", f"{val}", "-1"]
            hint = "כל מספר (שאינו 0) בחזקת 0 שווה למספר קבוע."
            exp = "לפי הגדרת החזקה, כל מספר שונה מאפס בחזקת 0 שווה ל-1."
        else:
            n = random.randint(2, 4)
            text = f"פשטו את הביטוי: (2x)^{n}. מה התוצאה?"
            ans = f"{2**n}x^{n}"
            dist = [f"2x^{n}", f"{2*n}x^{n}", f"{2**n}x"]
            hint = "מעלים בחזקה גם את המקדם המספרי וגם את המשתנה."
            exp = f"(2x)^{n} = 2^{n} · x^{n} = {2**n}x^{n}."
        opts, correct_idx = shuffle_options(ans, dist)
        qs.append({
            "id": qid, "text": text, "options": opts, "answer": correct_idx,
            "hint": hint, "explanation": exp, "topic": "חוקי חזקות נוספים"
        })
        qid += 1

    # 5. Short multiplication formulas (30 questions)
    for _ in range(30):
        sign = random.choice(["+", "-"])
        b = random.choice([1, 2, 3, 4, 5, 6, 7])
        b2 = b * b
        mid = 2 * b
        text = f"פתחו לפי נוסחאות הכפל המקוצר: (x {sign} {b})^2."
        ans = f"x^2 {sign} {mid}x + {b2}"
        other_sign = "-" if sign == "+" else "+"
        dist = [f"x^2 + {b2}", f"x^2 {sign} {b}x + {b2}", f"x^2 {other_sign} {mid}x + {b2}"]
        opts, correct_idx = shuffle_options(ans, dist)
        hint = "השתמשו בנוסחה: (a ± b)^2 = a^2 ± 2ab + b^2."
        exp = f"לפי נוסחת הכפל המקוצר: (x {sign} {b})^2 = x^2 {sign} 2·x·{b} + {b}^2 = {ans}."
        qs.append({
            "id": qid, "text": text, "options": opts, "answer": correct_idx,
            "hint": hint, "explanation": exp, "topic": "נוסחאות כפל מקוצר"
        })
        qid += 1

    # 6. Difference of squares (25 questions)
    for _ in range(25):
        b = random.choice([2, 3, 4, 5, 6, 7, 8, 9, 10])
        b2 = b * b
        text = f"פשטו את המכפלה: (x - {b})(x + {b}). למה היא שווה?"
        ans = f"x^2 - {b2}"
        dist = [f"x^2 + {b2}", f"x^2 - {2*b}x - {b2}", f"x^2 - {b}"]
        opts, correct_idx = shuffle_options(ans, dist)
        hint = "זוהי נוסחת הפרש ריבועים: (a - b)(a + b) = a^2 - b^2."
        exp = f"(x - {b})(x + {b}) = x^2 - {b}^2 = x^2 - {b2}."
        qs.append({
            "id": qid, "text": text, "options": opts, "answer": correct_idx,
            "hint": hint, "explanation": exp, "topic": "הפרש ריבועים"
        })
        qid += 1

    assert len(qs) == 150, f"Expected 150 questions, got {len(qs)}"
    with open(OUT_DIR / "03_algebraic_expressions_powers.json", "w", encoding="utf-8") as f:
        json.dump(qs, f, ensure_ascii=False, indent=2)
    print(f"Generated {len(qs)} questions in 03_algebraic_expressions_powers.json (IDs {qs[0]['id']}-{qs[-1]['id']})")

# --------------------------------------------------------------------------
# 05: GEOMETRY, PYTHAGORAS & ANGLES (200 questions: 6600 - 6799)
# -------------------------------------------------------------
def build_topic_05():
    qs = []
    qid = 6600

    triples = [(3, 4, 5), (6, 8, 10), (5, 12, 13), (9, 12, 15), (8, 15, 17), (12, 16, 20)]
    # 1. Hypotenuse (35 questions)
    for _ in range(35):
        a, b, c = random.choice(triples)
        text = f"במשולש ישר זווית אורכי הניצבים הם {a} ס\"מ ו-{b} ס\"מ. מהו אורך היתר?"
        ans = f"{c} ס\"מ"
        dist = [f"{a + b} ס\"מ", f"{c + 2} ס\"מ", f"{c - 1} ס\"מ"]
        opts, correct_idx = shuffle_options(ans, dist)
        hint = "השתמשו במשפט פיתגורס: a^2 + b^2 = c^2, כאשר c הוא היתר."
        exp = f"לפי פיתגורס: {a}^2 + {b}^2 = {a*a} + {b*b} = {c*c}. שורש של {c*c} הוא {c} ס\"מ."
        qs.append({
            "id": qid, "text": text, "options": opts, "answer": correct_idx,
            "hint": hint, "explanation": exp, "topic": "משפט פיתגורס - מציאת יתר"
        })
        qid += 1

    # 2. Leg (35 questions)
    for _ in range(35):
        a, b, c = random.choice(triples)
        text = f"במשולש ישר זווית אורך היתר הוא {c} ס\"מ ואורך אחד הניצבים {b} ס\"מ. מה אורך הניצב השני?"
        ans = f"{a} ס\"מ"
        dist = [f"{c - b} ס\"מ", f"{a + 3} ס\"מ", f"{round(math.sqrt(c*c + b*b))} ס\"מ"]
        opts, correct_idx = shuffle_options(ans, dist)
        hint = "משפט פיתגורס למציאת ניצב: a^2 = c^2 - b^2."
        exp = f"לפי פיתגורס: ניצב בריבוע = {c}^2 - {b}^2 = {c*c} - {b*b} = {a*a}. לכן הניצב הוא {a} ס\"מ."
        qs.append({
            "id": qid, "text": text, "options": opts, "answer": correct_idx,
            "hint": hint, "explanation": exp, "topic": "משפט פיתגורס - מציאת ניצב"
        })
        qid += 1

    # 3. Parallel lines angles (40 questions)
    for _ in range(40):
        angle = random.randint(35, 145)
        supp = 180 - angle
        rel = random.choice(["alternate", "corresponding", "interior", "vertical"])
        if rel == "alternate":
            text = f"בין שני ישרים מקבילים הנחתכים על ידי ישר שלישי, זווית אחת היא {angle}°. מה גודל הזווית המתחלפת לה?"
            ans = f"{angle}°"
            dist = [f"{supp}°", f"{90}°", f"{angle + 10}°"]
            hint = "זוויות מתחלפות בין ישרים מקבילים שוות זו לזו."
            exp = f"זוויות מתחלפות בין מקבילים שוות בגודלן, לכן גודלה הוא {angle}°."
        elif rel == "corresponding":
            text = f"בין שני ישרים מקבילים, זווית אחת היא {angle}°. מה גודל הזווית המתאימה לה?"
            ans = f"{angle}°"
            dist = [f"{supp}°", f"{180}°", f"{angle - 10}°"]
            hint = "זוויות מתאימות בין ישרים מקבילים שוות זו לזו."
            exp = f"זוויות מתאימות בין ישרים מקבילים שוות זו לזו, לכן התשובה היא {angle}°."
        elif rel == "interior":
            text = f"בין שני ישרים מקבילים, זווית אחת היא {angle}°. מה גודל הזווית החד-צדדית הפנימית לה?"
            ans = f"{supp}°"
            dist = [f"{angle}°", f"{90}°", f"{supp - 10}°"]
            hint = "סכום זוויות חד-צדדיות (פנימיות) בין ישרים מקבילים הוא 180°."
            exp = f"סכום זוויות חד-צדדיות בין מקבילים הוא 180°. 180° פחות {angle}° שווה {supp}°."
        else: # vertical
            text = f"שני ישרים נחתכים יוצרים זוויות קודקודיות. אם אחת היא {angle}°, מה גודל הזווית הקודקודית לה?"
            ans = f"{angle}°"
            dist = [f"{supp}°", f"{90}°", f"{180 - angle//2}°"]
            hint = "זוויות קודקודיות תמיד שוות זו לזו."
            exp = f"זוויות קודקודיות שוות תמיד בגודלן, לכן הזווית היא {angle}°."
        opts, correct_idx = shuffle_options(ans, dist)
        qs.append({
            "id": qid, "text": text, "options": opts, "answer": correct_idx,
            "hint": hint, "explanation": exp, "topic": "זוויות בין ישרים מקבילים"
        })
        qid += 1

    # 4. Triangle angles (40 questions)
    for _ in range(40):
        mode = random.choice(["third_angle", "isosceles_base", "isosceles_vertex", "exterior"])
        if mode == "third_angle":
            a1 = random.randint(30, 80)
            a2 = random.randint(30, 180 - a1 - 20)
            a3 = 180 - a1 - a2
            text = f"במשולש שתי זוויות שגודלן {a1}° ו-{a2}°. מה גודל הזווית השלישית?"
            ans = f"{a3}°"
            dist = [f"{a3 + 10}°", f"{a3 - 10}°", f"{180 - a1}°"]
            hint = "סכום הזוויות בכל משולש הוא תמיד 180°."
            exp = f"סכום הזוויות במשולש הוא 180°. 180° - {a1}° - {a2}° = {a3}°."
        elif mode == "isosceles_base":
            v = random.choice([40, 50, 60, 70, 80, 100])
            b = (180 - v) // 2
            text = f"במשולש שווה-שוקיים זווית הראש היא {v}°. מה גודל כל אחת מזוויות הבסיס?"
            ans = f"{b}°"
            dist = [f"{v}°", f"{180 - v}°", f"{b + 15}°"]
            hint = "במשולש שווה שוקיים זוויות הבסיס שוות זו לזו, וסכום כל הזוויות 180°."
            exp = f"זוויות הבסיס שוות: (180° - {v}°) / 2 = {b}°."
        elif mode == "isosceles_vertex":
            b = random.randint(35, 75)
            v = 180 - 2 * b
            text = f"במשולש שווה-שוקיים זווית הבסיס היא {b}°. מה גודל זווית הראש?"
            ans = f"{v}°"
            dist = [f"{b}°", f"{180 - b}°", f"{v + 10}°"]
            hint = "סכום זוויות הבסיס הוא פעמיים זווית הבסיס. חסרו מ-180°."
            exp = f"שתי זוויות הבסיס הן {b}°. זווית הראש: 180° - 2·{b}° = {v}°."
        else: # exterior
            a1 = random.randint(30, 70)
            a2 = random.randint(30, 70)
            ext = a1 + a2
            text = f"במשולש שתי זוויות פנימיות שאינן צמודות לזווית חיצונית הן {a1}° ו-{a2}°. מה גודל הזווית החיצונית?"
            ans = f"{ext}°"
            dist = [f"{180 - ext}°", f"{ext - 15}°", f"{a1}°"]
            hint = "זווית חיצונית למשולש שווה לסכום שתי הזוויות הפנימיות שאינן צמודות לה."
            exp = f"לפי משפט הזווית החיצונית: {a1}° + {a2}° = {ext}°."
        opts, correct_idx = shuffle_options(ans, dist)
        qs.append({
            "id": qid, "text": text, "options": opts, "answer": correct_idx,
            "hint": hint, "explanation": exp, "topic": "זוויות במשולש"
        })
        qid += 1

    # 5. Congruence & Quadrilaterals (50 questions)
    criteria = [
        ("צ.ז.צ (צלע-זווית-צלע)", "שתי צלעות והזווית הכלואה ביניהן"),
        ("ז.צ.ז (זווית-צלע-זווית)", "שתי זוויות והצלע שביניהן"),
        ("צ.צ.צ (צלע-צלע-צלע)", "שלוש צלעות שוות בהתאמה"),
        ("צ.צ.ז (צלע-צלע-זווית מול הצלע הגדולה)", "שתי צלעות וזווית מול הצלע הגדולה")
    ]
    for _ in range(25):
        c_name, c_desc = random.choice(criteria)
        text = f"לפי איזה משפט חפיפה חופפים שני משולשים אם שווים בהם: {c_desc}?"
        ans = c_name
        dist = [c[0] for c in criteria if c[0] != c_name]
        opts, correct_idx = shuffle_options(ans, dist)
        hint = "שימו לב האם הזווית כלואה בין הצלעות, או שהצלע שוכבת בין הזוויות."
        exp = f"התנאי '{c_desc}' תואם בדיוק למשפט החפיפה {c_name}."
        qs.append({
            "id": qid, "text": text, "options": opts, "answer": correct_idx,
            "hint": hint, "explanation": exp, "topic": "משפטי חפיפת משולשים"
        })
        qid += 1

    for _ in range(25):
        prop_type = random.choice(["opp_angles", "adj_angles", "diagonals", "area"])
        if prop_type == "opp_angles":
            a = random.randint(40, 140)
            text = f"במקבילית זווית אחת שווה ל-{a}°. מה גודל הזווית הנגדית לה?"
            ans = f"{a}°"
            dist = [f"{180 - a}°", f"{90}°", f"{360 - a}°"]
            hint = "במקבילית, זוויות נגדיות תמיד שוות זו לזו."
            exp = f"תכונה יסודית במקבילית: זוויות נגדיות שוות בגודלן, לכן התשובה {a}°."
        elif prop_type == "adj_angles":
            a = random.randint(40, 140)
            text = f"במקבילית זווית אחת שווה ל-{a}°. מה גודל הזווית הסמוכה לה?"
            ans = f"{180 - a}°"
            dist = [f"{a}°", f"{90}°", f"{180}°"]
            hint = "במקבילית, זוויות סמוכות משלימות ל-180° (זוויות חד-צדדיות בין מקבילים)."
            exp = f"סכום זוויות סמוכות במקבילית הוא 180°. 180° פחות {a}° שווה {180-a}°."
        elif prop_type == "diagonals":
            text = "איזו תכונה נכונה תמיד לגבי אלכסוני המקבילית?"
            ans = "האלכסונים חוצים זה את זה"
            dist = ["האלכסונים שווים זה לזה", "האלכסונים מאונכים זה לזה", "האלכסונים חוצים את הזוויות"]
            hint = "בכל מקבילית האלכסונים נפגשים בנקודת האמצע של כל אחד מהם."
            exp = "בכל מקבילית האלכסונים חוצים זה את זה. שוויון או ניצבות קיימים רק במלבן/מעוין."
        else:
            b = random.randint(5, 15)
            h = random.randint(4, 10)
            text = f"במקבילית אורך צלע הוא {b} ס\"מ והגובה לצלע זו הוא {h} ס\"מ. מהו שטח המקבילית?"
            ans = f"{b * h} סמ\"ר"
            dist = [f"{(b * h) // 2} סמ\"ר", f"{2 * (b + h)} סמ\"ר", f"{b * h + b} סמ\"ר"]
            hint = "שטח מקבילית מחושב כמכפלת צלע בגובה היורד אליה (בלי חילוק ב-2!)."
            exp = f"שטח מקבילית = צלע כפול גובה = {b} · {h} = {b * h} סמ\"ר."
        opts, correct_idx = shuffle_options(ans, dist)
        qs.append({
            "id": qid, "text": text, "options": opts, "answer": correct_idx,
            "hint": hint, "explanation": exp, "topic": "מרובעים ומקבילית"
        })
        qid += 1

    assert len(qs) == 200, f"Expected 200 questions, got {len(qs)}"
    with open(OUT_DIR / "05_geometry_pythagoras_angles.json", "w", encoding="utf-8") as f:
        json.dump(qs, f, ensure_ascii=False, indent=2)
    print(f"Generated {len(qs)} questions in 05_geometry_pythagoras_angles.json (IDs {qs[0]['id']}-{qs[-1]['id']})")

# --------------------------------------------------------------------------
# 06: WORD PROBLEMS, PERCENTAGES & RATIOS (200 questions: 6800 - 6999)
# -------------------------------------------------------------
def build_topic_06():
    qs = []
    qid = 6800

    # 1. Percentage increase / decrease (40 questions)
    for _ in range(40):
        orig = random.choice([50, 80, 100, 120, 150, 200, 250, 300, 400])
        pct = random.choice([10, 15, 20, 25, 30, 40, 50])
        kind = random.choice(["discount", "increase"])
        diff = int(orig * pct / 100)
        if kind == "discount":
            final_p = orig - diff
            text = f"מוצר שמחירו המקורי {orig} שקלים נמכר בהנחה של {pct}%. מה מחירו לאחר ההנחה?"
            ans = f"{final_p} שקלים"
            dist = [f"{orig - pct} שקלים", f"{orig + diff} שקלים", f"{final_p + 10} שקלים"]
            hint = "חשבו את גובה ההנחה בשקלים (מחיר מקורי כפול האחוז חלקי 100) וחסרו מהמחיר."
            exp = f"ההנחה: {orig} · {pct}% = {diff} שקלים. המחיר המוזל: {orig} - {diff} = {final_p} שקלים."
        else:
            final_p = orig + diff
            text = f"מחיר מוצר היה {orig} שקלים והתייקר ב-{pct}%. מה מחירו החדש?"
            ans = f"{final_p} שקלים"
            dist = [f"{orig + pct} שקלים", f"{orig - diff} שקלים", f"{final_p - 10} שקלים"]
            hint = "חשבו את סכום ההתייקרות והוסיפו אותו למחיר המקורי."
            exp = f"ההתייקרות: {orig} · {pct}% = {diff} שקלים. המחיר החדש: {orig} + {diff} = {final_p} שקלים."
        opts, correct_idx = shuffle_options(ans, dist)
        qs.append({
            "id": qid, "text": text, "options": opts, "answer": correct_idx,
            "hint": hint, "explanation": exp, "topic": "אחוזים - הנחה והתייקרות"
        })
        qid += 1

    # 2. Finding original price or percentage rate (35 questions)
    for _ in range(35):
        mode = random.choice(["rate", "original"])
        if mode == "rate":
            old_p = random.choice([50, 80, 100, 200, 400])
            pct = random.choice([10, 20, 25, 50])
            new_p = old_p + int(old_p * pct / 100)
            text = f"מחיר פריט עלה מ-{old_p} שקלים ל-{new_p} שקלים. מהו אחוז העלייה?"
            ans = f"{pct}%"
            dist = [f"{pct + 5}%", f"{new_p - old_p}%", f"{pct * 2}%"]
            hint = "חלקו את ההפרש במחיר במחיר המקורי, והכפילו ב-100%."
            exp = f"ההפרש הוא {new_p - old_p} שקלים. {new_p - old_p} חלקי {old_p} שווה {pct/100} = {pct}%."
        else:
            pct = random.choice([10, 20, 25, 50])
            part = random.choice([6, 10, 15, 20, 30])
            total = int(part / (pct / 100))
            text = f"אם {pct}% מכמות מסוימת שווים ל-{part}, מהי הכמות השלמה?"
            ans = f"{total}"
            dist = [f"{total + 10}", f"{part * pct}", f"{total - part}"]
            hint = "חלקו את הערך החלקי באחוז (או כפלו במספר המתאים להגעה ל-100%)."
            exp = f"כדי למצוא את השלם: {part} חלקי {pct}% = {part} / {pct/100} = {total}."
        opts, correct_idx = shuffle_options(ans, dist)
        qs.append({
            "id": qid, "text": text, "options": opts, "answer": correct_idx,
            "hint": hint, "explanation": exp, "topic": "חישובי אחוזים"
        })
        qid += 1

    # 3. Ratios and proportions (40 questions)
    for _ in range(40):
        r1 = random.randint(2, 5)
        r2 = random.randint(3, 7)
        if r1 == r2: r2 += 1
        mult = random.randint(3, 10)
        val1 = r1 * mult
        val2 = r2 * mult
        tot = val1 + val2
        q_type = random.choice(["find_part", "find_total", "scale"])
        if q_type == "find_part":
            text = f"היחס בין בנים לבנות בכיתה הוא {r1}:{r2}. אם יש בכיתה {val1} בנים, כמה בנות יש?"
            ans = f"{val2} בנות"
            dist = [f"{val2 + mult} בנות", f"{val1 + r2} בנות", f"{val2 - 2} בנות"]
            hint = "מצאו את ערכו של חלק יחס אחד: חלקו את מספר הבנים בנתון היחס שלהם."
            exp = f"גודל יחידת יחס אחת: {val1} / {r1} = {mult}. מספר הבנות: {r2} · {mult} = {val2}."
        elif q_type == "find_total":
            text = f"סכום של {tot} שקלים חולק בין שני אחים ביחס {r1}:{r2}. כמה קיבל האח עם החלק הגדול?"
            big_val = max(val1, val2)
            ans = f"{big_val} שקלים"
            dist = [f"{min(val1, val2)} שקלים", f"{tot // 2} שקלים", f"{big_val + 5} שקלים"]
            hint = "חברו את חלקי היחס ומצאו את ערכו של חלק יחס אחד מתוך הסכום הכולל."
            exp = f"סך כל חלקי היחס: {r1}+{r2}={r1+r2}. ערך חלק: {tot}/({r1+r2})={mult}. החלק הגדול: {max(r1,r2)}·{mult}={big_val}."
        else:
            text = f"במתכון לעוגה היחס בין קמח לסוכר הוא {r1}:{r2}. אם משתמשים ב-{val2} גרם סוכר, כמה גרם קמח נדרשים?"
            ans = f"{val1} גרם"
            dist = [f"{val1 + 10} גרם", f"{val2 - r1} גרם", f"{val1 * 2} גרם"]
            hint = "היחס נשמר: ערך הקמח חלקי {r1} שווה לערך הסוכר חלקי {r2}."
            exp = f"יחידת יחס אחת: {val2} / {r2} = {mult}. כמות הקמח: {r1} · {mult} = {val1} גרם."
        opts, correct_idx = shuffle_options(ans, dist)
        qs.append({
            "id": qid, "text": text, "options": opts, "answer": correct_idx,
            "hint": hint, "explanation": exp, "topic": "יחס ופרופורציה"
        })
        qid += 1

    # 4. Motion problems (45 questions)
    for _ in range(45):
        speed = random.choice([40, 50, 60, 70, 80, 90, 100])
        time_h = random.choice([2, 3, 4, 5])
        dist = speed * time_h
        m_type = random.choice(["find_dist", "find_time", "find_speed", "meeting"])
        if m_type == "find_dist":
            text = f"מכונית נסעה במהירות קבועה של {speed} קמ\"ש במשך {time_h} שעות. איזה מרחק עברה?"
            ans = f"{dist} ק\"מ"
            dist_opts = [f"{dist + speed} ק\"מ", f"{dist - 20} ק\"מ", f"{speed * 2} ק\"מ"]
            hint = "נוסחת התנועה: מרחק = מהירות כפול זמן."
            exp = f"מרחק = מהירות · זמן = {speed} · {time_h} = {dist} ק\"מ."
        elif m_type == "find_time":
            text = f"רכבת עברה מרחק של {dist} ק\"מ במהירות של {speed} קמ\"ש. כמה שעות ארכה הנסיעה?"
            ans = f"{time_h} שעות"
            dist_opts = [f"{time_h + 1} שעות", f"{time_h - 1} שעות", f"{time_h * 2} שעות"]
            hint = "נוסחת התנועה: זמן = מרחק חלקי מהירות."
            exp = f"זמן = מרחק / מהירות = {dist} / {speed} = {time_h} שעות."
        elif m_type == "find_speed":
            text = f"אופנוע עבר מרחק של {dist} ק\"מ בתוך {time_h} שעות. מה הייתה מהירותו הממוצעת?"
            ans = f"{speed} קמ\"ש"
            dist_opts = [f"{speed + 10} קמ\"ש", f"{speed - 10} קמ\"ש", f"{dist // 2} קמ\"ש"]
            hint = "נוסחת התנועה: מהירות = מרחק חלקי זמן."
            exp = f"מהירות = מרחק / זמן = {dist} / {time_h} = {speed} קמ\"ש."
        else: # meeting
            v1 = random.choice([60, 70, 80])
            v2 = random.choice([50, 70, 90])
            t = random.choice([2, 3])
            total_d = (v1 + v2) * t
            text = f"שני כלי רכב יצאו זה לקראת זה ונפגשו כעבור {t} שעות. מהירויותיהם {v1} ו-{v2} קמ\"ש. מה המרחק ההתחלתי ביניהם?"
            ans = f"{total_d} ק\"מ"
            dist_opts = [f"{total_d - v1} ק\"מ", f"{v1 * t} ק\"מ", f"{total_d + 30} ק\"מ"]
            hint = "בתנועה זה לקראת זה, סוכמים את המהירויות וכופלים בזמן הפגישה."
            exp = f"המהירות המשותפת: {v1}+{v2}={v1+v2} קמ\"ש. המרחק הכולל: ({v1}+{v2}) · {t} = {total_d} ק\"מ."
        opts, correct_idx = shuffle_options(ans, dist_opts)
        qs.append({
            "id": qid, "text": text, "options": opts, "answer": correct_idx,
            "hint": hint, "explanation": exp, "topic": "בעיות תנועה ומהירות"
        })
        qid += 1

    # 5. Work and rate problems (40 questions)
    for _ in range(40):
        rate_t = random.choice([2, 3, 4, 5, 6])
        hours = random.choice([1, 2, 3])
        if hours >= rate_t: hours = rate_t - 1
        if hours == 0: hours = 1
        g = math.gcd(hours, rate_t)
        frac_str = f"{hours//g}/{rate_t//g}"
        text = f"צינור ממלא בריכה שלמה בקצב קבוע תוך {rate_t} שעות. איזה חלק מהבריכה ימלא הצינור ב-{hours} שעות?"
        ans = frac_str
        dist = [f"1/{rate_t}", f"{hours}/{rate_t+1}", f"1/{hours+1}"]
        opts, correct_idx = shuffle_options(ans, dist)
        hint = "ההספק לשעה אחת הוא 1 חלקי זמן העבודה השלם. כפלו במספר השעות."
        exp = f"הצינור ממלא 1/{rate_t} מהבריכה בשעה. ב-{hours} שעות ימלא {hours}/{rate_t} = {frac_str} מהבריכה."
        qs.append({
            "id": qid, "text": text, "options": opts, "answer": correct_idx,
            "hint": hint, "explanation": exp, "topic": "בעיות הספק ועבודה"
        })
        qid += 1

    assert len(qs) == 200, f"Expected 200 questions, got {len(qs)}"
    with open(OUT_DIR / "06_word_problems_percentages_ratios.json", "w", encoding="utf-8") as f:
        json.dump(qs, f, ensure_ascii=False, indent=2)
    print(f"Generated {len(qs)} questions in 06_word_problems_percentages_ratios.json (IDs {qs[0]['id']}-{qs[-1]['id']})")

if __name__ == "__main__":
    build_topic_01()
    build_topic_03()
    build_topic_05()
    build_topic_06()

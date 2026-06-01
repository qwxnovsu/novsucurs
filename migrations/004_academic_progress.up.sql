CREATE TABLE IF NOT EXISTS "semesters" (
  "id" bigserial PRIMARY KEY,
  "group_name" varchar(255) NOT NULL REFERENCES "groups" ("name") ON DELETE CASCADE,
  "start_date" date NOT NULL,
  "end_date" date NOT NULL,
  "year" integer NOT NULL,
  "number" integer NOT NULL CHECK ("number" BETWEEN 1 AND 2),
  CONSTRAINT semesters_date_range_check CHECK ("end_date" >= "start_date"),
  CONSTRAINT semesters_group_year_number_unique UNIQUE ("group_name", "year", "number")
);

CREATE TABLE IF NOT EXISTS "grades" (
  "id" bigserial PRIMARY KEY,
  "student_id" bigint NOT NULL REFERENCES "students" ("id") ON DELETE CASCADE,
  "lesson_id" bigint NOT NULL REFERENCES "lessons" ("id") ON DELETE CASCADE,
  "grade" integer NOT NULL CHECK ("grade" BETWEEN 2 AND 5),
  "comment" text,
  "created_at" timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  "updated_at" timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  CONSTRAINT grades_student_lesson_unique UNIQUE ("student_id", "lesson_id")
);

CREATE TABLE IF NOT EXISTS "grade_criteria" (
  "id" bigserial PRIMARY KEY,
  "subject_id" bigint NOT NULL REFERENCES "subjects" ("id") ON DELETE CASCADE,
  "min_attendance_pct" numeric(5, 2) NOT NULL DEFAULT 70.00 CHECK ("min_attendance_pct" BETWEEN 0 AND 100),
  "min_avg_grade" numeric(4, 2) NOT NULL DEFAULT 3.00 CHECK ("min_avg_grade" BETWEEN 2 AND 5),
  CONSTRAINT grade_criteria_subject_unique UNIQUE ("subject_id")
);

INSERT INTO grade_criteria (subject_id, min_attendance_pct, min_avg_grade)
SELECT id, 70.00, 3.00
FROM subjects
ON CONFLICT (subject_id) DO NOTHING;

INSERT INTO semesters (group_name, start_date, end_date, year, number)
SELECT
    group_name,
    MIN(lesson_date_start::date) AS start_date,
    MAX(lesson_date_start::date) AS end_date,
    EXTRACT(YEAR FROM MIN(lesson_date_start))::integer AS year,
    1 AS number
FROM lessons
GROUP BY group_name
ON CONFLICT (group_name, year, number) DO UPDATE
SET start_date = EXCLUDED.start_date,
    end_date = EXCLUDED.end_date;

UPDATE lesson_attendance
SET attendance_status = CASE
    WHEN ((student_id + lesson_id) % 5) = 0 THEN 'absent'
    WHEN ((student_id + lesson_id) % 4) = 0 THEN 'late'
    ELSE 'present'
END;

INSERT INTO grades (student_id, lesson_id, grade, comment)
SELECT
    student_id,
    lesson_id,
    CASE
        WHEN ((student_id + lesson_id) % 6) = 0 THEN 5
        WHEN ((student_id + lesson_id) % 3) = 0 THEN 4
        ELSE 3
    END,
    'Оценка за занятие'
FROM lesson_attendance
WHERE attendance_status <> 'absent'
ON CONFLICT (student_id, lesson_id) DO NOTHING;

CREATE OR REPLACE FUNCTION touch_grade_updated_at () RETURNS TRIGGER AS $$
BEGIN
    NEW.updated_at = CURRENT_TIMESTAMP;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

DROP TRIGGER IF EXISTS trigger_touch_grade_updated_at ON grades;
CREATE TRIGGER trigger_touch_grade_updated_at BEFORE UPDATE ON grades FOR EACH ROW
EXECUTE FUNCTION touch_grade_updated_at ();

DROP FUNCTION IF EXISTS get_credit_status (varchar, bigint, date, date);

CREATE OR REPLACE FUNCTION get_credit_status (
  p_group_name varchar,
  p_subject_id bigint,
  p_date_start date,
  p_date_end date
) RETURNS TABLE (
  student_id bigint,
  student_user_id bigint,
  student_name text,
  group_name varchar,
  subject_name varchar,
  attendance_percentage numeric,
  average_grade numeric,
  min_attendance_pct numeric,
  min_avg_grade numeric,
  credit_passed boolean
) AS $$
BEGIN
    RETURN QUERY
    SELECT
        st.id AS student_id,
        u.id AS student_user_id,
        (u.second_name || ' ' || u.name) AS student_name,
        st.group_name,
        subj.name AS subject_name,
        CASE
            WHEN COUNT(DISTINCT la.lesson_id) = 0 THEN 0.00
            ELSE ROUND(
                COUNT(DISTINCT CASE WHEN la.attendance_status IN ('present', 'late') THEN la.lesson_id END)::numeric
                / COUNT(DISTINCT la.lesson_id)::numeric * 100,
                2
            )
        END AS attendance_percentage,
        COALESCE(ROUND(AVG(gr.grade)::numeric, 2), 0.00) AS average_grade,
        COALESCE(gc.min_attendance_pct, 70.00) AS min_attendance_pct,
        COALESCE(gc.min_avg_grade, 3.00) AS min_avg_grade,
        (
            CASE
                WHEN COUNT(DISTINCT la.lesson_id) = 0 THEN 0.00
                ELSE ROUND(
                    COUNT(DISTINCT CASE WHEN la.attendance_status IN ('present', 'late') THEN la.lesson_id END)::numeric
                    / COUNT(DISTINCT la.lesson_id)::numeric * 100,
                    2
                )
            END >= COALESCE(gc.min_attendance_pct, 70.00)
            AND COALESCE(ROUND(AVG(gr.grade)::numeric, 2), 0.00) >= COALESCE(gc.min_avg_grade, 3.00)
        ) AS credit_passed
    FROM students st
    JOIN users u ON u.id = st.user_id
    JOIN lessons l ON l.group_name = st.group_name
    JOIN subjects subj ON subj.id = l.subject_id
    LEFT JOIN lesson_attendance la ON la.lesson_id = l.id AND la.student_id = st.id
    LEFT JOIN grades gr ON gr.lesson_id = l.id AND gr.student_id = st.id
    LEFT JOIN grade_criteria gc ON gc.subject_id = subj.id
    WHERE
        st.group_name = p_group_name
        AND subj.id = p_subject_id
        AND l.lesson_date_start::date BETWEEN p_date_start AND p_date_end
    GROUP BY
        st.id,
        u.id,
        u.second_name,
        u.name,
        st.group_name,
        subj.name,
        gc.min_attendance_pct,
        gc.min_avg_grade
    ORDER BY
        u.second_name,
        u.name;
END;
$$ LANGUAGE plpgsql;

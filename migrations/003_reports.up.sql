CREATE OR REPLACE FUNCTION get_student_attendance_report (
  p_group_name varchar,
  p_date_start date,
  p_date_end date
) RETURNS TABLE (
  student_id bigint,
  student_name text,
  group_name varchar,
  total_lessons bigint,
  present_count bigint,
  late_count bigint,
  absent_count bigint,
  attendance_percentage numeric
) AS $$
BEGIN
    RETURN QUERY
    SELECT
        s.id AS student_id,
        (u.second_name || ' ' || u.name) AS student_name,
        s.group_name,
        COUNT(la.lesson_id) AS total_lessons,
        COUNT(CASE WHEN la.attendance_status = 'present' THEN 1 END) AS present_count,
        COUNT(CASE WHEN la.attendance_status = 'late' THEN 1 END) AS late_count,
        COUNT(CASE WHEN la.attendance_status = 'absent' THEN 1 END) AS absent_count,
        CASE
            WHEN COUNT(la.lesson_id) = 0 THEN 0.0
            ELSE ROUND(
                (COUNT(CASE WHEN la.attendance_status IN ('present', 'late') THEN 1 END)::numeric /
                 COUNT(la.lesson_id)::numeric) * 100, 2
            )
        END AS attendance_percentage
    FROM
        students s
    JOIN
        users u ON s.user_id = u.id
    JOIN
        lesson_attendance la ON s.id = la.student_id
    JOIN
        lessons l ON la.lesson_id = l.id
    WHERE
        (p_group_name IS NULL OR s.group_name = p_group_name)
        AND l.lesson_date_start::date BETWEEN p_date_start AND p_date_end
    GROUP BY
        s.id, u.second_name, u.name, s.group_name
    ORDER BY
        s.group_name, u.second_name, u.name;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION get_course_attendance_stats (p_date_start date, p_date_end date) RETURNS TABLE (
  course_number integer,
  avg_attendance_percentage numeric,
  total_absences bigint
) AS $$
BEGIN
    RETURN QUERY
    SELECT
        (EXTRACT(YEAR FROM CURRENT_DATE)::integer - g.start_year + 1),
        ROUND(AVG(CASE WHEN sc.total = 0 THEN 0 ELSE (sc.present_late::numeric / sc.total::numeric) * 100 END), 2),
        SUM(sc.absent_c)::bigint
    FROM groups g
    JOIN lessons l ON g.name = l.group_name
    JOIN (
        SELECT la.lesson_id, COUNT(*) as total,
               COUNT(CASE WHEN la.attendance_status IN ('present', 'late') THEN 1 END) as present_late,
               COUNT(CASE WHEN la.attendance_status = 'absent' THEN 1 END) as absent_c
        FROM lesson_attendance la GROUP BY la.lesson_id
    ) sc ON l.id = sc.lesson_id
    WHERE l.lesson_date_start::date BETWEEN p_date_start AND p_date_end
    GROUP BY 1 ORDER BY 1;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION get_group_attendance_stats (p_date_start date, p_date_end date) RETURNS TABLE (
  group_name varchar,
  total_lessons_conducted bigint,
  avg_attendance_percentage numeric,
  total_absences bigint
) AS $$
BEGIN
    RETURN QUERY
    SELECT
        g.name,
        COUNT(DISTINCT l.id),
        ROUND(AVG(CASE WHEN sc.total = 0 THEN 0 ELSE (sc.present_late::numeric / sc.total::numeric) * 100 END), 2),
        SUM(sc.absent_c)::bigint
    FROM groups g
    JOIN lessons l ON g.name = l.group_name
    JOIN (
        SELECT la.lesson_id, COUNT(*) as total,
               COUNT(CASE WHEN la.attendance_status IN ('present', 'late') THEN 1 END) as present_late,
               COUNT(CASE WHEN la.attendance_status = 'absent' THEN 1 END) as absent_c
        FROM lesson_attendance la GROUP BY la.lesson_id
    ) sc ON l.id = sc.lesson_id
    WHERE l.lesson_date_start::date BETWEEN p_date_start AND p_date_end
    GROUP BY g.name ORDER BY 3 DESC;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION auto_mark_absence_on_lesson_creation () RETURNS TRIGGER AS $$
BEGIN
    INSERT INTO lesson_attendance (lesson_id, student_id, attendance_status)
    SELECT
        NEW.id,
        s.id,
        'absent'
    FROM
        students s
    WHERE
        s.group_name = NEW.group_name;

    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER trigger_auto_mark_absence
AFTER INSERT ON lessons FOR EACH ROW
EXECUTE FUNCTION auto_mark_absence_on_lesson_creation ();

CREATE OR REPLACE FUNCTION get_available_groups_for_user (p_user_id bigint) RETURNS TABLE (group_name varchar) AS $$
DECLARE
    v_user_role varchar;
BEGIN
    SELECT r.type INTO v_user_role
    FROM users u
    JOIN roles r ON u.role_id = r.id
    WHERE u.id = p_user_id;

    IF v_user_role = 'admin' THEN
        RETURN QUERY
        SELECT g.name FROM groups g ORDER BY g.name;

    ELSIF v_user_role = 'teacher' THEN
        RETURN QUERY
        SELECT DISTINCT l.group_name
        FROM lessons l
        WHERE l.teacher_id = p_user_id
        ORDER BY l.group_name;

    ELSIF v_user_role = 'student' THEN
        RETURN QUERY
        SELECT s.group_name
        FROM students s
        WHERE s.user_id = p_user_id;

    END IF;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION get_available_students_for_user (p_user_id bigint) RETURNS SETOF student_view AS $$
DECLARE
    v_user_role varchar;
BEGIN
    SELECT r.type INTO v_user_role
    FROM users u
    JOIN roles r ON u.role_id = r.id
    WHERE u.id = p_user_id;

    IF v_user_role = 'admin' THEN
        RETURN QUERY
        SELECT * FROM student_view
        ORDER BY group_name, second_name, name;

    ELSIF v_user_role = 'teacher' THEN
        RETURN QUERY
        SELECT * FROM student_view sv
        WHERE sv.group_name IN (
            SELECT DISTINCT l.group_name
            FROM lessons l
            WHERE l.teacher_id = p_user_id
        )
        ORDER BY sv.group_name, sv.second_name, sv.name;

    ELSIF v_user_role = 'student' THEN
        RETURN QUERY
        SELECT * FROM student_view sv
        WHERE sv.id = p_user_id;

    END IF;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION current_app_user_id () RETURNS BIGINT AS $$
BEGIN
    RETURN current_setting('app.user_id', true)::BIGINT;
EXCEPTION WHEN OTHERS THEN
    RETURN NULL;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE VIEW available_students_view AS
SELECT
  *
FROM
  get_available_students_for_user (current_app_user_id ());

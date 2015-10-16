
maintime:

    int tick_x = ((m_tick % m_ppqn) * (m_window_x - 1)) / m_ppqn ;
    int beat_x = (((m_tick / 4) % m_ppqn) * width) / m_ppqn ;
    int bar_x = (((m_tick / 16) % m_ppqn) * width) / m_ppqn ;

        m_window->draw_rectangle
        (
            m_gc, true, 2, /*tick_x + 2,*/ 2, m_window_x - 4, m_window_y - 4
        );

mainwid:

    int slots = m_mainwnd_rows * m_mainwnd_cols;
    for (int s = 0; s < slots; s++)
    {
        int offset = (m_screenset * slots) + s;
        draw_sequence_on_pixmap(offset);
        m_last_tick_x[offset] = 0;
    }

bool
mainwid::valid_sequence (int seqnum)
{
    int slots = m_mainwnd_rows * m_mainwnd_cols;
    return seqnum >= (m_screenset * slots) && seqnum < ((m_screenset+1) * slots);
}

void
mainwid::calculate_base_sizes (int seqnum, int & basex, int & basey)
{
    int i = (seqnum / m_mainwnd_rows) % m_mainwnd_cols;
    int j =  seqnum % m_mainwnd_rows;
    basex = m_mainwid_border + (m_seqarea_x + m_mainwid_spacing) * i;
    basey = m_mainwid_border + (m_seqarea_y + m_mainwid_spacing) * j;
}

    int slots = m_mainwnd_rows * m_mainwnd_cols;
    int offset = m_screenset * slots;
    for (int s = 0; s < slots; s++)
        draw_marker_on_sequence(offset + s, ticks);

int mainwid::seq_from_xy (int a_x, int a_y)


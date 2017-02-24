
bool
extract_port_names
(
    const std::string & fullname,
    std::string & clientname,
    std::string & portname
)
{
    bool result = ! fullname.empty();
    clientname.clear();
    portname.clear();
    if (result)
    {
        std::string cname;
        std::string pname;
        std::size_t colonpos = fullname.find_first_of(":"); /* not last! */
        if (colonpos != std::string::npos)
        {
            /*
             * The client name consists of all characters up the the first
             * colon.  Period.  The port name consists of all characters
             * after that colon.  Period.
             */

            cname = fullname.substr(0, colonpos);
#if 0
            if (! cname.empty())
            {
                std::size_t spacepos = cname.find_first_of(" ");
                if (spacepos != std::string::npos)
                {
                    spacepos = cname.find_first_of(" ", spacepos+1);
                    if (spacepos != std::string::npos)
                    {
                        std::size_t colonpos = cname.find_last_of(":");
                        if (colonpos != std::string::npos)
                           cname = cname.substr(spacepos+1, colonpos);
                    }
                }
            }
#endif  // 0
            pname = fullname.substr(colonpos+1);
            result = ! cname.empty() && ! pname.empty();
        }
        else
            pname = fullname;

        clientname = cname;
        portname = pname;
    }
    return result;
}

/**
 *  Extracts the buss name from "bus:port".  Sometimes we don't need both
 *  parts at once.
 *
 *  However, when a2jmidid is active. the port name will have a colon in it.
 *
 * \param fullname
 *      The "bus:port" name.
 *
 * \return
 *      Returns the "bus" portion of the string.  If there is no colon, then
 *      it is assumed there is no buss name, so an empty string is returned.
 */

std::string
extract_bus_name (const std::string & fullname)
{
    std::size_t colonpos = fullname.find_first_of(":");  /* not last! */
    return (colonpos != std::string::npos) ?
        fullname.substr(0, colonpos) : std::string("");
}

/**
 *  Extracts the port name from "bus:port".  Sometimes we don't need both
 *  parts at once.
 *
 *  However, when a2jmidid is active. the port name will have a colon in it.
 *
 * \param fullname
 *      The "bus:port" name.
 *
 * \return
 *      Returns the "port" portion of the string.  If there is no colon, then
 *      it is assumed that the name is a port name, and so \a fullname is
 *      returned.
 */

std::string
extract_port_name (const std::string & fullname)
{
    std::size_t colonpos = fullname.find_first_of(":");  /* not last! */
    return (colonpos != std::string::npos) ?
        fullname.substr(colonpos + 1) : fullname ;
}

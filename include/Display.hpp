#pragma once

#include <dnskeeper.h>

// The HTML assumes 4 column rows. Changes to 
// this requires modifications to the HTML 
// templates inside app/etc
using row_t = std::vector<std::string>;

inline const std::string& error_page() {
    static const std::string err = R"(
        <HTML>
            <HEAD>
                <title>Internal server error</title>
            </HEAD>
            <BODY>
                The page encountered a rendering error
            </BODY>
        </HTML>
    )";
    return err;
}

class TablePage
{
private:
    std::string m_template;
    std::string m_page;
    std::vector<std::string> m_rows;

protected:
    unsigned m_column_count = 0; // Verifies row data
    std::string get_template(const std::string& filename)
    {
        std::string data;
        if (std::filesystem::exists(filename))
        {
            std::string line;
            std::ifstream fil(filename);
            if (fil.is_open())
            {
                while (getline(fil, line))
                    data += line;
                fil.close();
            }
            return data;
        } else {
            return "";
        }
    }

    bool inject(std::string& var, 
                const std::string& placeholder, 
                const std::string& data,
                bool replace = false)
    {
        bool found = false;
        size_t start_pos = 0;
        while ((start_pos = var.find(placeholder, start_pos)) != std::string::npos)
        {
            found = true;
            if(replace) {
                var.replace(start_pos, placeholder.length(), data);
                start_pos += data.length();
            }
            else{
                var.insert(start_pos, data);
                start_pos += data.length() + placeholder.length();
            }
        }
        return found;
    }

public:
    TablePage(const char *title, const char *subtitle, row_t colheaders)
    {
        m_template = get_template("./www/table_page.tpl");
        if(m_template.empty()) {
            LOG(ERROR) << "Page template " << title << " not found\n";
            exit(-1);
        }

        // Build up the headers
        std::string rowdata = "";
        for (const auto& cell : colheaders) {
            rowdata += "<th>";
            rowdata += cell;
            rowdata += "</th>\n";
            m_column_count++;
        }

        // Setup the title and subtitle
        if(inject(m_template, "<!-- TITLE -->", title, true)
            && inject(m_template, "<!-- SUBTITLE -->", subtitle, true)
            && inject(m_template, "<!-- HEADERS -->", rowdata))
        {
            LOG(DEBUG) << "Page template " << title << " initialized\n";
        } else {
            // An injection failure is a critical code issue with the template
            LOG(ERROR) << "Page template " << title << " failed initialization\n";
            exit(-1);
        }

        m_page = m_template;
    }

    void add_row(row_t row, bool highlight = false)
    {
        std::string rowdata = "\n<tr>";

        if(row.size() != m_column_count) {
            // Issue with the data
            LOG(ERROR) << "Column header count does not match row data\n";
            exit(-1);
        }

        if (highlight)
            rowdata = "<tr class=\"flagged\">";

        for (const auto& cell : row)
        {
            rowdata += "<td>";
            rowdata += cell;
            rowdata += "</td>";
        }

        rowdata += "</tr>";
        m_rows.push_back(rowdata);
    }

    void clear()
    {
        m_page = m_template;
        m_rows.clear();
    }

    std::string render()
    {
        for (const auto &tblrow : m_rows)
            if(!inject(m_page, "<!-- ROWS -->", tblrow))
                return error_page();
        return m_page;
    }
};

class ServerUI : public TablePage
{
private:
    bool valid_name(const std::string& data);
    bool valid_domain(const std::string& data);
    bool valid_ip(const std::string& data);

    void add_row(const std::string& name, 
                 const std::string& domain, 
                 const std::string& ip, 
                 const std::string& operation,
                 const std::string& title,
                 const std::string& text);

public:
    ServerUI()
        : TablePage("Servers",
                    "Servers in the database",
                    {"Friendly Name", "Cluster", "DNS status", "Actions"})
    {
    }

    using TablePage::clear;
    using TablePage::render;
    using TablePage::add_row;


    void addition(const std::string &name, const std::string &domain, const std::string &ip);
    void removal(const std::string &name, const std::string &domain, const std::string &ip);
};

class DnsUI : public TablePage
{
public:
    DnsUI()
        : TablePage("DNS Records",
                    "Currently published DNS entries",
                    {"Domain String", "IP", "Server Friendly Name", "Cluster Name"})
    {
    }

    using TablePage::add_row;
    using TablePage::clear;
    using TablePage::render;
};
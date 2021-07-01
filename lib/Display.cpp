#include <Display.hpp>
#include <fmt/core.h>

// Validation ref: https://docs.microsoft.com/en-us/troubleshoot/windows-server/identity/naming-conventions-for-computer-domain-site-ou

bool ServerUI::valid_name(const std::string& data) {
    // The FQDN needs to be 255 bytes in total minus 63 for the domain
    const int max_len = (255-63);
    return std::all_of(data.begin(), data.end(),
        [](char c) {
            return std::isalnum(c)
                || c == '-';
        }) && data.length() < max_len;
}

bool ServerUI::valid_domain(const std::string& data) {
    // Alphanumeric, minus, period and less than 63 chars
    return std::all_of(data.begin(), data.end(),
        [](char c) {
            return std::isalnum(c)
                || c == '-'
                || c == '.';
        }) && data.length() < 63;
}

bool ServerUI::valid_ip(const std::string& data) {
    return std::all_of(data.begin(), data.end(),
            [](char c) {
                return std::isdigit(c)
                    || c == '.';
            }) && data.length() <= 15; // ###:###:###:### (assumption: we dont do IPv6)
}

void ServerUI::add_row(const std::string& name, 
                       const std::string& domain, 
                       const std::string& ip, 
                       const std::string& operation,
                       const std::string& title,
                       const std::string& text)
{
    if(valid_name(name) && valid_domain(domain) && valid_ip(ip)) {
        const char *url = R"(<div onclick="innerHTML='<div class=blink>Pending</div>';async_once(this, '/{}?name={}&domain={}&ip={}');"><A href='javascript:void(0)' title="{}">{}</A></div>)";
        std::string rowdata = fmt::format(url, operation, name, domain, ip, title, text);
        add_row({name, 
                 domain.substr(0, domain.find('.')), 
                 (operation=="remove"?ip:"NONE"), 
                 rowdata});
    } else {
        LOG(WARNING) << "Suspect input detected (ignored) in request ["
                    << operation << "|"
                    << name << "|"
                    << domain << "|"
                    << ip << "]\n";
    }
}

void ServerUI::addition(const std::string &name, const std::string &domain, const std::string &ip)
{
    add_row(name, domain, ip, "add", "add to rotation", "Add");
}

void ServerUI::removal(const std::string &name, const std::string &domain, const std::string &ip)
{
    add_row(name, domain, ip, "remove", "remove from rotation", "Remove");
}
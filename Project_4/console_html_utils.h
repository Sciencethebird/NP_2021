#include <string>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <boost/asio.hpp>
#include <sstream>
#include <map>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/format.hpp>


std::string build_main_html(std::string console_head_html, std::string console_body_html){
    boost::format main_html_template
    = boost::format(
            "<!DOCTYPE html>\n"
            "<html lang=\"en\">\n"
            "  <head>\n"
            "    <meta charset=\"UTF-8\" />\n"
            "    <title>NP Project 3 Sample Console</title>\n"
            "    <link\n"
            "      rel=\"stylesheet\"\n"
            "      href=\"https://cdn.jsdelivr.net/npm/bootstrap@4.5.3/dist/css/bootstrap.min.css\"\n"
            "      integrity=\"sha384-TX8t27EcRE3e/ihU7zmQxVncDAy5uIKz4rEkgIXeMed4M0jlfIDPvg6uqKI2xXr2\"\n"
            "      crossorigin=\"anonymous\"\n"
            "    />\n"
            "    <link\n"
            "      href=\"https://fonts.googleapis.com/css?family=Source+Code+Pro\"\n"
            "      rel=\"stylesheet\"\n"
            "    />\n"
            "    <link\n"
            "      rel=\"icon\"\n"
            "      type=\"image/png\"\n"
            "      href=\"https://cdn0.iconfinder.com/data/icons/small-n-flat/24/678068-terminal-512.png\"\n"
            "    />\n"
            "    <style>\n"
            "      * {\n"
            "        font-family: 'Source Code Pro', monospace;\n"
            "        font-size: 1rem !important;\n"
            "      }\n"
            "      body {\n"
            "        background-color: #212529;\n"
            "      }\n"
            "      pre {\n"
            "        color: #cccccc;\n"
            "      }\n"
            "      b {\n"
            "        color: #01b468;\n"
            "      }\n"
            "    </style>\n"
            "  </head>\n"
            "  <body>\n"
            "    <table class=\"table table-dark table-bordered\">\n"
            "      <thead>\n"
            "        <tr>\n"
            "%1%"
            "        </tr>\n"
            "      </thead>\n"
            "      <tbody>\n"
            "        <tr>\n"
            "%2%"
            "        </tr>\n"
            "      </tbody>\n"
            "    </table>\n"
            "  </body>\n"
            "</html>\n");
    main_html_template = main_html_template % console_head_html % console_body_html;
    return main_html_template.str();
}

std::string format_head_html(std::string ip_address, std::string port){
    return "<th scope=\"col\">" + ip_address + ":" + port +"</th>\n";
}
std::string format_body_html(int host_id){
    return "<td><pre id=\"s" + std::to_string(host_id) + "\" class=\"mb-0\"></pre></td>\n";
}

// removes dangerous characters in the html
void html_escape(std::string& content){
    boost::replace_all(content, "&", "&amp;");
    boost::replace_all(content, "\"", "&quot;");
    boost::replace_all(content, "\'", "&apos;");
    boost::replace_all(content, "<", "&lt;");
    boost::replace_all(content, ">", "&gt;");
    boost::replace_all(content, "\r", "");
    boost::replace_all(content, "\n", "&NewLine;");
}
void output_shell(std::string host_id, std::string content){
    html_escape(content);
    std::string output = "<script>document.getElementById('"+ host_id +"').innerHTML += '" + content + "';</script>";
    std::cout << output;
    std::cout.flush();
}
void output_command(std::string host_id, std::string content){
    html_escape(content);
    std::string output = "<script>document.getElementById('"+ host_id +"').innerHTML += '<b>" + content + "</b>';</script>";
    std::cout << output;
    std::cout.flush();
}
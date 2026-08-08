#pragma once

#include <string>
#include <QPushButton>

class QValidator;
class QWidget;

class ButtonEditable : public QPushButton {
 public:
  using parser_t = std::function<std::string(const std::string)>;
  using get_str_t = std::function<std::string()>;
  ButtonEditable(
    const std::string &initial_button_value,
    QWidget *parent,
    const std::string &dialog_title,
    const std::string &dialog_prompt,
    get_str_t get_edit_value,
    QValidator *validator,
    parser_t parse);
 private:
   void Edit();
   std::string initial_button_value_;
   std::string dialog_title_;
   std::string dialog_prompt_;
   get_str_t get_edit_value_;
   QValidator *validator_;
   parser_t parse_;
};

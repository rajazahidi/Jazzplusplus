#ifndef JZ_NAMEDVALUECHOICE_H
#define JZ_NAMEDVALUECHOICE_H

class tNamedValue;

class tNamedValueChoice : public wxChoice
{
  public:

   tNamedValueChoice(wxWindow *parent, tNamedValue *values);

   long GetValue();

   void SetValue(long measure);

  private:

    tNamedValue* values;
};

#endif // !defined(JZ_NAMEDVALUECHOICE_H)
